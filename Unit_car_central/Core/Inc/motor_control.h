/**
 * @file motor_control.h
 * @brief Servo 및 DC 모터 제어를 위한 함수 프로토타입과 타입을 정의한다.
 * @author YeonsuJ
 * @date 2025-07-22
 */

#ifndef INC_MOTOR_CONTROL_H_
#define INC_MOTOR_CONTROL_H_

#include "main.h"
#include "rf_handler.h" // VehicleCommand_t 구조체를 사용하기 위해 포함

/**
 * @brief 모터의 회전 방향을 나타내는 열거형
 */
typedef enum {
    DIRECTION_BACKWARD = 0,
    DIRECTION_FORWARD = 1
} MotorDirection_t;

/**
 * @brief Servo, DC 모터 제어에 필요한 모든 주변장치를 초기화한다.
 * @note 이 함수는 PWM 타이머 채널을 활성화하고, 모터의 초기 방향을 설정한다.
 */
void MotorControl_Init(void);

/**
 * @brief 수신된 주행 명령에 따라 모든 모터의 상태를 업데이트한다.
 * @param command CommHandler로부터 받은 주행 명령 구조체의 포인터
 * @note 이 함수는 내부적으로 서보, DC모터, 방향 제어 함수를 모두 호출한다.
 */
void MotorControl_Update(const VehicleCommand_t* command);

/**
 * @brief 서보 모터의 각도를 제어한다.
 * @param roll 조종기에서 수신된 roll 값 (-90.0 ~ 90.0)이며, 이 값은 스티어링 각도로 변환된다.
 */
void Control_Servo(float roll);

/**
 * @brief DC 모터의 속도를 제어한다.
 * @param accel_ms 가속 버튼이 눌린 시간 (ms)
 * @param brake_ms 브레이크 버튼이 눌린 시간 (ms)
 * @note 가속, 브레이크, 관성 주행 로직을 포함한다.
 */
void Control_DcMotor(uint16_t accel_ms, uint16_t brake_ms);

/**
 * @brief DC 모터의 회전 방향을 업데이트한다.
 * @param direction 설정할 새로운 모터 방향 (DIRECTION_FORWARD 또는 DIRECTION_BACKWARD)
 * @note 이전 상태와 비교하여 방향이 변경되었을 때만 GPIO를 제어한다.
 */
void Update_MotorDirection(MotorDirection_t direction);

#endif /* INC_MOTOR_CONTROL_H_ */
