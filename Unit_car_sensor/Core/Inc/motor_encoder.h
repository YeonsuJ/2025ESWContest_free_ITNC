/**
 * @file motor_encoder.h
 * @brief DC 모터 엔코더 관련 함수를 선언한다.
 * @author YeonsuJ
 * @date 2025-07-25
 */

#ifndef INC_MOTOR_ENCODER_H_
#define INC_MOTOR_ENCODER_H_


/**
 * @brief 현재 모터의 RPM 값을 반환한다.
 * @retval float 현재 RPM 값
 */
float MotorControl_GetRPM(void);

/**
 * @brief DWT(Data Watchpoint and Trace) 유닛을 초기화한다.
 * @note 정밀한 시간 측정을 위해 사용된다.
 */
void DWT_Init(void);

/**
 * @brief 주기적으로 호출되어 모터의 RPM을 계산하고 갱신한다.
 */
void Update_Motor_RPM(void);


#endif /* INC_MOTOR_ENCODER_H_ */
