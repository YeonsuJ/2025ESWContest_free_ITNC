# Unit_controller MCU 

## 1. 시스템 개요 및 역할

**Controller MCU**는 차량을 원격으로 조종하기 위한 **핵심 입력 및 송신 장치 역할**을 수행합니다. 내장된 MPU6050 자이로 센서로부터 사용자의 기울기 값을 측정하고, 엑셀, 브레이크, 방향 전환 버튼의 입력을 실시간으로 감지합니다. 이 모든 제어 데이터는 NRF24L01 무선 통신 모듈을 통해 차량으로 전송됩니다. 또한, 수신 측으로부터 받은 ACK(응답) 패킷을 분석하여 현재 차량의 속도, 주행 방향 등 주요 상태 정보를 OLED 디스플레이에 시각적으로 표시하며, 통신 연결 상태를 사용자에게 직관적으로 피드백하는 핵심 제어 모듈입니다.

---

## 2. 주요 소스 파일 및 함수 설명 

### [main.c](./Unit_controller/Core/Src/main.c)
MCU의 시작점(Entry Point)으로, 하드웨어 초기화 및 FreeRTOS 스케줄러를 실행합니다.

- **`main()`**
  - **역할**: HAL 드라이버와 시스템 클럭을 초기화하고, GPIO, I2C, SPI, 타이머 등 필요한 모든 주변 장치를 설정합니다. `InputHandler`(입력), `CommHandler`(통신), MPU6050(센서), SSD1306(OLED) 등 각 모듈을 초기화한 뒤 FreeRTOS 커널을 시작하여 시스템의 제어권을 태스크에 넘깁니다.

- **`HAL_GPIO_EXTI_Callback()`** / **`HAL_TIM_PeriodElapsedCallback()`**
  - **역할**: 하드웨어 인터럽트 발생 시 호출되는 콜백 함수들입니다. GPIO 핀 인터럽트(버튼 입력, NRF24 수신)와 타이머 인터럽트(버튼 누름 시간 측정)가 발생하면, 해당 이벤트를 처리할 FreeRTOS 태스크나 관련 핸들러(`InputHandler`)에 작업을 위임합니다.

### [freertos.c](./Unit_controller/Core/Src/freertos.c)
시스템의 핵심 로직을 담당하는 FreeRTOS 태스크들을 정의하고 구현합니다.

- **`StartsensorTask()`**
  - **역할**: **센서 데이터 측정 태스크**입니다. 주기적으로 MPU6050 센서로부터 roll 각도 값을 읽어와 `sensorQueue`라는 메시지 큐에 전송합니다. 이 태스크는 센서 데이터 생성을 전담합니다.
- **`StartcommTask()`**
  - **역할**: **데이터 송신 태스크**입니다. sensorQueue에 데이터가 들어올 때까지 대기하다가, sensorTask가 측정한 roll 값을 수신합니다. 수신된 데이터와 현재 버튼 입력 상태를 종합하여 전송용 패킷을 생성하고, CommHandler를 통해 차량으로 무선 전송합니다.
- **`StartackHandlerTask()`**
  - **역할**: **무선 통신 결과 처리 태스크**입니다. 평소에는 휴면 상태로 대기하다가, NRF24 모듈로부터 송신 완료 또는 실패 인터럽트가 발생하면 세마포어(ackSemHandle)에 의해 즉시 활성화됩니다. 통신 상태를 확인하여 성공 시 수신된 ACK 패킷(차량 상태 정보)을 처리하고, 실패 시 통신 두절 상태를 시스템에 알립니다.
- **`StartDisplayTask()`**
  - **역할**: **사용자 인터페이스 출력 태스크**입니다. 주기적으로 시스템의 상태(차량 속도, 방향, 통신 상태)를 공유 데이터 영역에서 읽어와 OLED 디스플레이에 렌더링합니다. 통신이 실패하면 "NO SIGNAL" 경고를 표시하고, 정상이면 현재 속도를 퍼센트로 변환하여 출력하는 등 모든 시각적 피드백을 담당합니다.

### [input_handler.c](./Unit_controller/Core/Src/input_handler.c) / [input_handler.h](./Unit_controller/Core/Inc/input_handler.h)
GPIO와 타이머 인터럽트를 기반으로 사용자의 버튼 입력을 처리합니다.

- **`InputHandler_Init()`**
  - **역할**: 엑셀, 브레이크 등 버튼 상태를 저장하는 내부 변수들을 초기화합니다.
- **`InputHandler_GpioCallback()`**
  - **역할**: 버튼이 눌리거나 떼어지는 순간 호출되는 인터럽트 기반 콜백입니다. 버튼의 현재 상태(눌림/떼짐)를 static 변수에 즉시 기록합니다.
- **`InputHandler_TimerCallback()`**
  - **역할**: 20ms마다 주기적으로 호출되어, 특정 버튼이 계속 눌려있는 동안 해당 버튼의 누적 카운트를 1씩 증가시킵니다.
- **`InputHandler_GetAccelMillis()`** / **`InputHandler_GetBrakeMillis()`**
  - **역할**: `TimerCallback`에서 누적된 카운트 값에 20을 곱하여, 버튼이 눌린 총 시간을 밀리초(ms) 단위로 계산하여 반환합니다.

### [comm_handler.c](./Unit_controller/Core/Src/comm_handler.c) / [comm_handler.h](./Unit_controller/Core/Inc/comm_handler.h)
NRF24L01 무선 통신 모듈의 저수준(low-level) 제어를 담당합니다.

- **`CommHandler_Init()`**
  - **역할**: NRF24 모듈의 채널, 데이터 속도, 주소, 재전송 횟수 등 모든 통신 파라미터를 설정하고 송신 모드로 초기화합니다.
- **`CommHandler_Transmit()`**
  - **역할**: 상위 태스크(`commTask`)로부터 전송할 데이터 패킷을 받아 NRF24 모듈의 하드웨어 버퍼에 쓰고, 실질적인 전송을 명령합니다.
- **`CommHandler_CheckStatus()`**
  - **역할**: `ackHandlerTask`에 의해 호출되며, NRF24의 상태 레지스터를 읽어 마지막 통신 시도의 결과를 반환합니다. **전송 성공(TX_DS), 전송 실패(MAX_RT)** 상태를 구분하고, 성공 시에는 ACK와 함께 수신된 데이터 페이로드를 버퍼에서 읽어오는 역할까지 수행합니다.

---

## 3. 활용한 외부 라이브러리 설명

### 3-1. MPU6050 Library

`MPU6050.c` / `MPU6050.h`

자이로/가속도 센서와의 I2C 통신을 위한 드라이버 라이브러리입니다. 센서 초기화 및 데이터 레지스터 값(가속도, 각속도)을 읽어오는 함수들을 제공합니다. 이 프로젝트에서는 라이브러리를 통해 얻은 roll 각도 값을 차량 조향 데이터로 활용합니다.

> 출처: https://github.com/leech001/MPU6050

### 3-2. stm32_hal_nrf24_library (MIT License)

NRF24L01+ 무선 통신 모듈 제어를 위한 라이브러리입니다. 복잡한 레지스터 제어와 SPI 통신 과정을 추상화하여, 개발자가 직관적인 API를 통해 무선 통신 기능을 쉽게 구현할 수 있도록 돕습니다.

- `NRF24.c` / `NRF24.h` (핵심 드라이버): 라이브러리의 핵심 엔진으로, nrf24_init(), nrf24_transmit() 등 개발자가 직접 사용하는 공용 함수(API)와 실제 동작 로직을 담고 있습니다.

- `NRF24_conf.h` (하드웨어 설정): 사용자의 보드 환경에 맞게 SPI 포트와 CE, CSN 핀 정보를 정의하는 설정 파일입니다.

- `NRF24_reg_addresses.h` (레지스터 맵): NRF24 칩 내부 레지스터 주소에 STATUS, FLUSH_TX와 같이 사람이 읽기 쉬운 별명을 부여하여 코드의 가독성과 유지보수성을 향상시킵니다.

> 출처: https://github.com/developer328/stm32_hal_nrf24_library


### 3-3. SSD1306 OLED Display Library

`ssd1306.c` / `ssd1306.h`

`fonts.c` / `fonts.h`

위 파일들은 SSD1306 기반의 OLED 디스플레이 제어를 위한 오픈 소스 라이브러리 세트입니다.

ssd1306은 I2C 통신을 통해 그래픽을 출력하는 핵심 드라이버로, 화면의 저수준(low-level) 제어를 담당합니다. fonts 라이브러리는 텍스트 출력에 필요한 폰트 비트맵 데이터를 제공하며, ssd1306 드라이버는 이 데이터를 참조하여 화면에 문자를 그려냅니다.

> 출처 : <br>https://www.micropeta.com/ssd1306.c <br> https://www.micropeta.com/ssd1306.h <br> <https://www.micropeta.com/fonts.c> <br> https://www.micropeta.com/fonts.h
