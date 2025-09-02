/**
 * @file motor_control.c
 * @brief Servo 및 DC 모터 제어 함수의 구체적인 로직을 구현한다.
 * @author YeonsuJ
 * @date 2025-07-22
 * @note 타이머(TIM1, TIM2)와 GPIOA 핀을 사용하여 모터를 제어한다.
 */
#include "motor_control.h"

// === 타이머 및 GPIO 외부 참조 ===
/**
 * @brief DC 모터 PWM 제어용 타이머 핸들 (TIM1)
 */
extern TIM_HandleTypeDef htim1;
/**
 * @brief 서보 모터 PWM 제어용 타이머 핸들 (TIM2)
 */
extern TIM_HandleTypeDef htim2;

// === private 매크로 ===
/**
 * @brief DC 모터를 정방향으로 회전시키기 위한 GPIO 제어 매크로
 */
#define MOTOR_FORWARD()     do { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET); HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); } while (0)
/**
 * @brief DC 모터를 역방향으로 회전시키기 위한 GPIO 제어 매크로
 */
#define MOTOR_BACKWARD()    do { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); } while (0)


// === DC 모터 제어를 위한 새로운 매크로 ===
#define MAX_DUTY            1000    ///< PWM 최대 듀티 값 (htim1의 ARR 값에 맞춰 설정한다)
#define MIN_DUTY_ON_ACCEL   400     ///< 가속 시작 시 최소 듀티 (모터가 실제로 회전하기 시작하는 최소 PWM 값).
#define ACCEL_SENSITIVITY   0.8f    ///< 가속 민감도. 값이 클수록 가속 버튼 유지 시간에 비해 속도가 빠르게 증가한다.
#define COAST_DECREMENT     3       ///< 관성 주행 시 듀티 감소량. 이 값만큼 듀티가 서서히 감소한다.
#define BRAKE_STEP          500     ///< 브레이크 시 듀티 감소량. 이 값만큼 듀티가 급격히 감소한다.

/**
 * @brief Servo, DC 모터 제어에 필요한 모든 주변장치를 초기화한다.
 * @note 각 모터에 연결된 PWM 타이머 채널을 시작하고, 초기 방향을 전진으로 설정한다.
 */
void MotorControl_Init(void)
{
    // PWM 타이머 시작
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

    // 초기 방향 설정
    MOTOR_FORWARD();
}

/**
 * @brief 수신된 주행 명령에 따라 모든 모터의 상태를 업데이트한다.
 * @param command RF 통신으로 수신된 주행 명령 데이터 구조체의 포인터
 */
void MotorControl_Update(const VehicleCommand_t* command)
{
	Update_MotorDirection((MotorDirection_t)command->direction);
    Control_Servo(command->roll);
    Control_DcMotor(command->accel_ms, command->brake_ms);
}

/**
 * @brief 서보 모터의 각도를 제어한다.
 * @param roll 조종기에서 수신된 roll 값. 유효 범위는 -90.0 ~ 90.0 이다.
 * @note roll 값을 PWM 듀티 사이클 값(CCR)으로 변환하여 타이머 레지스터에 설정한다.
 * 물리적 서보 모터의 동작 범위에 맞춰 PWM 값을 1200~1900 사이로 제한한다.
 */
void Control_Servo(float roll)
{
    if (roll < -90.0f) roll = -90.0f;
    if (roll >  90.0f) roll =  90.0f;
    float angle = roll + 90.0f;
    uint16_t pwm_servo = (uint16_t)(1200 + (angle / 180.0f) * (1900 - 1200));
    if (pwm_servo < 1200) pwm_servo = 1200;
    if (pwm_servo > 1900) pwm_servo = 1900;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, pwm_servo);
}

/**
 * @brief DC 모터의 속도를 제어한다.
 * @param accel_ms 가속 버튼이 눌린 시간 (밀리초)
 * @param brake_ms 브레이크 버튼이 눌린 시간 (밀리초)
 * @note 이 함수는 `static` 변수 `current_duty`를 사용하여 현재 모터의 속도(듀티)를 기억하고,
 * 입력 값에 따라 점진적으로 속도를 조절한다.
 * - 브레이크 입력 시: 듀티를 `BRAKE_STEP` 만큼 급격히 감소시킨다.
 * - 가속 입력 시: 눌린 시간을 기반으로 목표 듀티를 계산하여 즉시 반영한다.
 * - 입력 없을 시: 듀티를 `COAST_DECREMENT` 만큼 서서히 감소시켜 관성 주행을 구현한다.
 */
void Control_DcMotor(uint16_t accel_ms, uint16_t brake_ms)
{
    static int16_t current_duty = 0;

    if (brake_ms > 0)
    {
        // 브레이크 버튼이 눌리면 빠르게 감속한다.
        current_duty -= BRAKE_STEP;
    }
    else if (accel_ms > 0)
    {
        // 가속 버튼이 눌리면, 눌린 시간을 기반으로 목표 듀티를 계산한다.
        // 오래 누를수록 target_duty 값이 커진다.
        int16_t target_duty = (int16_t)(MIN_DUTY_ON_ACCEL + (accel_ms * ACCEL_SENSITIVITY));

        // 현재 듀티를 목표 듀티로 설정한다.
        // (만약 더 부드러운 가속을 원한다면, current_duty를 target_duty까지 점진적으로 증가시키는 로직을 추가할 수 있다.)
        current_duty = target_duty;
    }
    else // 아무 버튼도 눌리지 않았을 때
    {
        // 서서히 속도를 줄여 관성 주행(Coasting)을 구현한다.
        current_duty -= COAST_DECREMENT;
    }

    // 듀티 값이 유효한 범위를 벗어나지 않도록 제한한다.
    if (current_duty > MAX_DUTY)
    {
        current_duty = MAX_DUTY;
    }
    if (current_duty < 0)
    {
        current_duty = 0;
    }

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_duty);
}

/**
 * @brief DC 모터의 회전 방향을 업데이트한다.
 * @param direction 설정할 새로운 모터 방향
 * @note `static` 변수 `prev_direction`을 사용하여 이전 방향 상태를 저장한다.
 * 새로운 방향이 이전 방향과 다를 경우에만 GPIO 제어 매크로를 호출하여
 * 불필요한 GPIO 쓰기 동작을 방지한다.
 */
void Update_MotorDirection(MotorDirection_t direction)
{
    static MotorDirection_t prev_direction = DIRECTION_FORWARD;
    if (direction != prev_direction) {
        prev_direction = direction;
        if (direction == DIRECTION_FORWARD) {
            MOTOR_FORWARD();
        } else {
            MOTOR_BACKWARD();
        }
    }
}