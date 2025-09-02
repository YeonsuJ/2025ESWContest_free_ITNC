# Unit_car_sensor ECU

## 1. 시스템 개요 및 역할

**Sensor ECU**는 차량 시스템의 **실시간 데이터 수집 및 전송 장치** 역할을 수행합니다. 차량의 물리적 상태를 감지하기 위해 모터 엔코더, 전·후방 초음파 센서, 조도 센서와 직접 인터페이스합니다. FreeRTOS 기반으로 동작하며, 각 센서로부터 정밀한 주기에 맞춰 데이터를 수집하고 노이즈 필터링 등 1차 가공을 수행합니다. 최종적으로 이 모든 원시 데이터를 CAN 통신 프로토콜에 맞게 패키징하여 다른 ECU(Status, Central)로 전송하는 핵심 데이터 소스 모듈입니다.

---

## 2. 주요 소스 파일 및 함수 설명

### [main.c](./Unit_car_sensor/Core/Src/main.c)
MCU의 시작점(Entry Point)으로, 하드웨어 초기화 및 FreeRTOS 스케줄러를 실행합니다.

- **`main()`**
    - **역할**: HAL 드라이버와 시스템 클럭을 초기화하고, GPIO, CAN, 및 센서 구동에 필요한 모든 타이머(TIM1-Encoder, TIM2/4-Ultrasonic)를 설정합니다. RPM의 정밀한 시간 측정을 위한 `DWT_Init`를 호출한 뒤, FreeRTOS 커널과 태스크를 시작시켜 시스템의 제어권을 넘깁니다.

### [freertos.c](./Unit_car_sensor/Core/Src/freertos.c)
시스템의 핵심 로직을 담당하는 FreeRTOS 태스크들을 정의하고 구현합니다.

- **`StartSensorTask()`**
    - **역할**: **주기적 데이터 수집 및 생산 태스크**입니다. 10ms의 정밀한 주기로 동작하며, `Update_Motor_RPM()`을 호출하여 RPM을 계산하고 `Ultrasonic_Trigger()`를 통해 거리 측정을 시작시킵니다. 수집된 모든 센서 데이터를 `SensorData_t` 구조체로 패키징하여 `CANTask`로 전송합니다.
- **`StartCANTask()`**
    - **역할**: **데이터 가공 및 전송 태스크**입니다. `SensorTask`로부터 데이터가 수신될 때만 동작하는 이벤트 기반 태스크입니다. 데이터를 수신하면 CAN 프로토콜에 맞게 가공(거리→비트마스크, RPM→2바이트 분리)하고 `TxData` 버퍼에 저장한 뒤, `CAN_Send()`를 호출하여 전송합니다.

### [can_handler.c](./Unit_car_sensor/Core/Src/can_handler.c) / [can_handler.h](./Unit_car_sensor/Core/Inc/can_handler.h)
CAN 통신의 초기 설정과 데이터 전송 기능을 담당합니다.

- **`CAN_tx_Init()`**
    - **역할**: CAN 컨트롤러를 활성화하고, 전송할 메시지의 헤더(`TxHeader`)를 설정합니다. 메시지 ID(`0x6A5`), 데이터 길이(DLC) 등 고정된 값을 미리 구성합니다.
- **`CAN_Send()`**
    - **역할**: `CANTask`에 의해 가공된 데이터가 저장된 `TxData` 버퍼의 내용을 CAN 버스를 통해 물리적으로 전송합니다.

### [motor_encoder.c](./Unit_car_sensor/Core/Src/motor_encoder.c) / [motor_encoder.h](./Unit_car_sensor/Core/Inc/motor_encoder.h)
타이머 엔코더 모드를 사용하여 모터의 RPM을 측정합니다.

- **`DWT_Init()`**
    - **역할**: RPM 계산 시 시간 변화량을 정밀하게 측정하기 위해 ARM Cortex-M 코어의 DWT Cycle Counter를 활성화합니다.
- **`Update_Motor_RPM()`**
    - **역할**: 엔코더 카운터 값의 변화량과 `DWT_Init`으로 활성화된 카운터를 통해 측정한 경과 시간을 이용하여 RPM을 계산합니다. **저주-통과 필터(Low-pass filter)**를 적용하여 측정값의 노이즈를 줄이고 안정적인 결과를 출력합니다.
- **`MotorControl_GetRPM()`**
    - **역할**: `SensorTask`에서 최종적으로 필터링된 RPM 값을 조회할 때 사용하는 Getter 함수입니다.

### [ultrasonic.c](./Unit_car_sensor/Core/Src/ultrasonic.c) / [ultrasonic.h](./Unit_car_sensor/Core/Inc/ultrasonic.h)
타이머 입력 캡처(Input Capture)를 이용해 초음파 센서의 거리를 측정합니다.

- **`Ultrasonic_Init()`**
    - **역할**: 초음파 센서 구동에 필요한 타이머(TIM2-delay_us, TIM4-Input Capture)를 활성화하고 입력 캡처 인터럽트를 시작합니다.
- **`Ultrasonic_Trigger()`**
    - **역할**: 전방 및 후방 센서의 Trigger 핀에 10µs 펄스를 전송하여 거리 측정을 시작하도록 명령합니다.
- **`HAL_TIM_IC_CaptureCallback()`**
    - **역할**: Echo 신호가 감지될 때 하드웨어적으로 호출되는 **인터럽트 서비스 루틴(ISR)**입니다. Echo 펄스의 상승-하강 엣지 사이 시간을 측정하여 거리를 cm 단위로 계산하고, 전역 변수(`distance_front`, `distance_rear`)를 직접 업데이트합니다.
