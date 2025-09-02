# Unit_car_status ECU 

## 1. 시스템 개요 및 역할

**Status ECU**는 차량 시스템의 **중앙 데이터 허브 및 상태 표시 장치** 역할을 수행합니다. CAN 통신을 통해 다른 ECU(Sensor, Central)로부터 차량의 핵심 상태 정보(주행 방향, 브레이크, 조도 등)를 수신합니다. 수신된 데이터를 바탕으로 통신 네트워크의 상태를 실시간으로 진단하고, 자체 배터리 잔량을 모니터링합니다. 최종적으로 이 모든 정보를 종합하여 LED와 OLED 디스플레이를 통해 사용자에게 직관적인 시각적 피드백을 제공하는 핵심 모듈입니다.

---

## 2. 주요 소스 파일 및 함수 설명 

### [main.c](./Unit_car_status/Core/Src/main.c)
MCU의 시작점(Entry Point)으로, 하드웨어 초기화 및 FreeRTOS 스케줄러를 실행합니다.

- **`main()`**
  - **역할**: HAL 드라이버와 시스템 클럭을 초기화하고, GPIO, CAN, I2C, ADC 등 필요한 모든 주변 장치를 설정합니다. OLED 드라이버를 초기화하고 CAN 통신 타임아웃 감지를 위한 초기 타임스탬프를 설정한 뒤, FreeRTOS 커널과 태스크를 시작시켜 시스템의 제어권을 넘깁니다.

### [freertos.c](./Unit_car_status/Core/Src/freertos.c)
시스템의 핵심 로직을 담당하는 FreeRTOS 태스크들을 정의하고 구현합니다.

- **`StartCANTask()`**
  - **역할**: **데이터 처리 및 통신 진단 태스크**입니다. 20ms 주기로 동작하며, CAN 수신 인터럽트가 큐에 넣어준 메시지들을 처리합니다. 메시지 ID( `0x6A5`, `0x321` )를 분석하여 최신 차량 상태를 갱신하고, 각 노드로부터 메시지가 수신되지 않으면 타임아웃으로 간주하여 통신 실패 상태를 진단합니다. 처리된 최종 데이터는 `DisplayTask`로 전송됩니다.
- **`StartDisplayTask()`**
  - **역할**: **사용자 인터페이스 출력 태스크**입니다. `CANTask`로부터 데이터가 수신될 때만 동작하는 이벤트 기반 태스크입니다. 데이터 수신 즉시 LED 상태를 업데이트하여 즉각적인 피드백을 제공하고, 50ms 주기로 OLED 디스플레이에 배터리 잔량 및 전체 통신 상태를 출력합니다.

### [can_handler.c](./Unit_car_status/Core/Src/can_handler.c) / [can_handler.h](./Unit_car_status/Core/Inc/can_handler.h)
CAN 통신의 초기 설정과 하드웨어 인터럽트 처리를 담당합니다.

- **`CANHandler_Init()`**
  - **역할**: CAN 컨트롤러를 활성화하고, 수신 메시지를 필터링하는 설정을 적용한 뒤, CAN 메시지 수신 인터럽트를 활성화합니다.
- **`CAN_Filter_Config()`**
  - **역할**: CAN 하드웨어 필터를 설정합니다. 현재 코드는 모든 ID의 메시지를 수신하도록 설정되어 있습니다.
- **`HAL_CAN_RxFifo1MsgPendingCallback()`**
  - **역할**: CAN 메시지 수신 시 하드웨어적으로 호출되는 **인터럽트 서비스 루틴(ISR)**입니다. 수신된 메시지를 하드웨어 버퍼에서 읽어 FreeRTOS 메시지 큐(`CANRxQueueHandle`)에 안전하게 전달하는 역할만 수행합니다.

### [led_control.c](./Unit_car_status/Core/Src/led_control.c) / [led_control.h](./Unit_car_status/Core/Inc/led_control.h)
차량의 LED 점등을 제어하는 간단한 인터페이스를 제공합니다.

- **`LEDControl_Update()`**
  - **역할**: 조도(`ldr`), 방향(`direction`), 브레이크(`brake`) 상태 값을 인자로 받아 차량의 모든 LED 상태를 갱신합니다. 함수 호출 시 먼저 모든 LED를 끈 다음, 입력된 조건에 맞는 전조등과 브레이크등을 다시 점등시킵니다.

### [oled_display.c](./Unit_car_status/Core/Src/oled_display.c) / [oled_display.h](./Unit_car_status/Core/Inc/oled_display.h)
I2C 통신 기반의 OLED 디스플레이 출력을 관리합니다.

- **`OLED_Init()`**
  - **역할**: SSD1306 OLED 드라이버를 초기화하고 화면을 깨끗하게 지웁니다.
- **`OLED_UpdateDisplay()`**
  - **역할**: 배터리 잔량, 전압, CAN 및 RF 통신 상태를 인자로 받아 화면 전체를 새로 그립니다. 통신이 실패하면 "CAN FAIL", "RF FAIL"과 같은 경고 메시지를 표시하고, 통신이 정상이면 배터리 정보를 표시합니다.

### [battery_monitor.c](./Unit_car_status/Core/Src/battery_monitor.c) / [battery_monitor.h](./Unit_car_status/Core/Inc/battery_monitor.h)
ADC를 사용하여 보드의 배터리 전압을 측정하고 관리합니다.

- **`Read_Battery_Percentage()`**
  - **역할**: 배터리 잔량을 **백분율(%)**로 변환하여 반환하는 메인 함수입니다. ADC 값을 읽고, 보정 계수를 적용한 뒤, **이동 평균 필터**를 통해 노이즈를 제거합니다. 최종적으로 전압 분배 법칙을 역산하여 실제 배터리 전압(Vbat)을 계산하고, 이를 0~100% 범위의 값으로 변환합니다.
- **`Get_Averaged_Vout()`**
  - **역할**: `Read_Battery_Percentage` 내부에서 사용되는 함수로, 최근 10개의 ADC 측정값을 저장하고 평균을 내어 안정적인 전압 값을 제공하는 **이동 평균 필터** 로직을 구현합니다.

---

## 3. 활용한 외부 라이브러리 설명

출처 : 
- [ssd1306.c](https://www.micropeta.com/ssd1306.c) / [ssd1306.h](https://www.micropeta.com/ssd1306.h)

- [fonts.c](https://www.micropeta.com/fonts.c) / [fonts.h](https://www.micropeta.com/fonts.h)

위 파일들은 SSD1306 기반의 OLED 디스플레이 제어를 위한 오픈 소스 라이브러리 세트입니다.

ssd1306은 I2C 통신을 통해 그래픽을 출력하는 핵심 드라이버로, 화면의 저수준(low-level) 제어를 담당합니다. fonts 라이브러리는 텍스트 출력에 필요한 폰트 비트맵 데이터를 제공하며, ssd1306 드라이버는 이 데이터를 참조하여 화면에 문자를 그려냅니다.

프로젝트의 `oled_display.c` 모듈은 이 라이브러리들을 사용하여 모든 시각적 정보를 효과적으로 표시합니다.
