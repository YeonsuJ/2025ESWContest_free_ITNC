# Central ECU

## 1. 시스템 개요 및 역할

**Central ECU**는 차량 시스템의 **두뇌 및 중앙 제어 장치** 역할을 수행합니다. 조종기로부터 **RF 무선 통신**을 통해 주행 명령(조향, 가감속)을 수신하고, 이를 즉시 해석하여 차량의 **DC 모터와 조향 서보를 직접 구동**합니다. 동시에 **CAN 게이트웨이**로서, 다른 ECU(Sensor)로부터 차량의 센서 데이터(RPM, 장애물 경고)를 수신하고, 자신의 주행 상태(방향, 브레이크)를 CAN 버스로 전파합니다. 최종적으로 수신한 센서 데이터를 조종기에 ACK Payload로 피드백하여 **양방향 통신 루프**를 완성하는 핵심 제어 모듈입니다.

---

## 2. 주요 소스 파일 및 함수 설명

### [main.c](./Core/Src/main.c)
MCU의 시작점(Entry Point)으로, 하드웨어 초기화 및 FreeRTOS 스케줄러를 실행합니다.

- **`main()`**
  - **역할**: HAL 드라이버와 시스템 클럭을 초기화하고, GPIO, CAN, SPI, TIM 등 필요한 모든 주변 장치를 설정합니다. RFHandler, CANHandler, MotorControl 등 각 기능 모듈을 초기화한 뒤, FreeRTOS 커널과 태스크를 시작시켜 시스템의 제어권을 넘깁니다.

### [freertos.c](./Core/Src/freertos.c)
시스템의 핵심 로직을 담당하는 FreeRTOS 태스크들을 정의하고 구현합니다.

- **`StartRFTask()`**
  - **역할**: **핵심 제어 및 명령 처리 태스크**입니다. RF 수신 인터럽트가 발생할 때만 동작하는 이벤트 기반 태스크로, 수신된 주행 명령을 즉시 해석하여 모터 제어를 요청합니다. 또한, CAN으로 수신된 센서 데이터를 조종기로 보낼 ACK 페이로드에 반영하고, 현재 차량 상태를 `CANTask`로 전달하는 총괄 제어 역할을 수행합니다.
- **`StartCANTask()`**
  - **역할**: **CAN 게이트웨이 및 상태 전파 태스크**입니다. RFTask로부터 차량의 주행 상태를 전달받을 때만 동작하며, 해당 정보를 CAN 버스를 통해 다른 ECU로 브로드캐스팅하는 역할을 담당합니다.

### [can_handler.c](./Core/Src/can_handler.c) / [can_handler.h](./Core/Inc/can_handler.h)
CAN 통신의 초기 설정과 하드웨어 인터럽트 처리를 담당합니다.

- **`CANHandler_Init()`**
  - **역할**: CAN 컨트롤러를 활성화하고, 수신 메시지를 필터링하는 설정을 적용한 뒤, CAN 메시지 수신 인터럽트를 활성화합니다.
- **`CAN_Filter_Config()`**
  - **역할**: CAN 하드웨어 필터를 설정하여, ID 0x6A5를 가진 센서 ECU의 메시지만을 수신하도록 제한합니다.
- **`HAL_CAN_RxFifo1MsgPendingCallback()`**
  - **역할**: CAN 메시지 수신 시 하드웨어적으로 호출되는 **인터럽트 서비스 루틴(ISR)**입니다. 수신된 메시지(RPM, 거리 신호)를 하드웨어 버퍼에서 읽어 FreeRTOS 메시지 큐(`CANRxQueueHandle`)에 안전하게 전달하는 역할만 수행합니다.
- **CAN_Send_DriveStatus()**
  - **역할**: 차량의 현재 상태(방향, 브레이크, RF 상태)를 인자로 받아 CAN 프레임으로 패키징한 후, CAN 버스로 전송합니다.

### [rf_handler.c](./Core/Src/rf_handler.c) / [rf_handler.h](./Core/Inc/rf_handler.h)
NRF24L01+ 모듈을 이용한 조종기와의 RF 통신을 관리합니다.

- **`RFHandler_Init()`**
  - **역할**: NRF24 모듈을 수신(Rx) 모드로 초기화하고, 통신 채널, 주소, 데이터 속도 등 통신 파라미터를 설정합니다.
- **`RFHandler_GetNewCommand()`**
  - **역할**: RF 수신이 완료되었는지 확인하고, 수신된 데이터 패킷을 파싱하여 VehicleCommand_t 구조체로 변환합니다. 또한, 이 함수가 호출될 때 미리 RFHandler_SetAckPayload로 설정된 ACK 데이터를 조종기로 자동 전송합니다.
- **`RFHandler_SetAckPayload()`**
  - **역할**: CAN으로 수신한 센서 데이터(RPM, 장애물 경고 등)를 ACK 전송 버퍼에 미리 로드하여, 다음 수신 성공 시 조종기로 피드백을 보낼 수 있도록 준비합니다.
- **`RFHandler_IrqCallback()`**
  - **역할**: RF 모듈의 IRQ 핀 인터럽트 발생 시 호출되어, 대기 중인 RFTask를 깨우기 위해 세마포어를 반환하는 신호 역할을 합니다.

### [motor_control.c](./Core/Src/motor_control.c) / [motor_control.h](./Core/Inc/motor_control.h)
차량의 물리적 구동(모터, 서보)을 직접 제어하는 인터페이스를 제공합니다.

- **`MotorControl_Init()`**
  - **역할**: DC 모터와 서보 모터 제어에 필요한 PWM 타이머를 시작하고, 모터의 초기 방향을 설정합니다.
- **`MotorControl_Update()`**
  - **역할**: VehicleCommand_t 구조체를 인자로 받아, 그 안에 담긴 조향, 가감속, 방향 명령에 따라 관련된 모든 모터 제어 함수를 호출하는 메인 인터페이스입니다.
- **`Control_DcMotor()`**
  - **역할**: 가속 및 브레이크 명령(accel_ms, brake_ms)에 따라 DC 모터의 PWM 듀티를 조절합니다. 관성 주행(Coasting) 및 급제동 로직을 포함하여 자연스러운 속도 제어를 구현합니다.
- **`Control_Servo()`**
  - **역할**: 조향 값(roll)을 서보 모터의 각도에 맞는 PWM 신호로 변환하여 스티어링을 제어합니다.

---

## 3. 활용한 외부 라이브러리 설명

이 프로젝트는 NRF24L01+ 무선 통신 모듈 제어를 위해 MIT 라이선스를 따르는 **stm32_hal_nrf24_library** 를 활용했습니다. 이 라이브러리는 복잡한 레지스터 제어와 SPI 통신 과정을 추상화하여, 개발자가 직관적인 API를 통해 무선 통신 기능을 쉽게 구현할 수 있도록 돕습니다.

> 출처: https://github.com/developer328/stm32_hal_nrf24_library (MIT License)

### 주요 구성 파일 및 역할

- `NRF24.c` / `NRF24.h` (핵심 드라이버)

- **역할**: 라이브러리의 핵심 엔진입니다. NRF24.h 파일은 nrf24_init(), nrf24_listen(), nrf24_receive() 등 개발자가 직접 호출하여 사용하는 **공용 함수(API)**들을 정의합니다. NRF24.c 파일은 이 함수들의 실제 동작 로직을 담고 있으며, 저수준 SPI 데이터 송수신과 레지스터 읽기/쓰기 같은 복잡한 과정을 모두 처리합니다.

- `NRF24_conf.h` (하드웨어 설정)
- **역할**: 라이브러리와 실제 STM32 하드웨어 간의 연결 다리 역할을 합니다. 개발자는 이 파일에 자신의 보드에 맞게 NRF24 모듈이 연결된 SPI 포트와 CE, CSN 핀 정보를 정의합니다. 이 덕분에 라이브러리의 핵심 코드를 수정하지 않고도 다양한 하드웨어 환경에 쉽게 이식할 수 있습니다.

- `NRF24_reg_addresses.h` (레지스터 맵)
- **역할**: NRF24L01+ 칩 내부의 수많은 레지스터 주소와 명령어 코드에 대해 사람이 읽기 쉬운 별명을 붙여놓은 파일입니다. 예를 들어, 0x07이라는 주소 대신 STATUS라는 이름으로, 0b11100001이라는 명령어 대신 FLUSH_TX라는 이름으로 접근할 수 있게 해줍니다. 이를 통해 코드의 가독성과 유지보수성을 크게 향상시킵니다.
