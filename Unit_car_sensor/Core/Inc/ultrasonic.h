
/**
 * @file ultrasonic.h
 * @brief 초음파 센서 관련 함수 및 변수를 선언한다.
 * @author YeonsuJ
 * @date 2025-07-26
 */

#ifndef INC_ULTRASONIC_H_
#define INC_ULTRASONIC_H_

#include "main.h"

// --- 외부 전역 변수 ---
extern volatile uint32_t distance_front; // 측정된 전방 거리 값 (cm)
extern volatile uint32_t distance_rear;  // 측정된 후방 거리 값 (cm)

/**
 * @brief 마이크로초(us) 단위의 지연을 생성한다.
 * @param us 지연시킬 시간 (마이크로초)
 */
void delay_us(uint16_t us);

/**
 * @brief 초음파 센서 사용에 필요한 하드웨어를 초기화한다.
 */
void Ultrasonic_Init(void);

/**
 * @brief 초음파 센서의 거리 측정을 시작시킨다.
 */
void Ultrasonic_Trigger(void);

#endif /* INC_ULTRASONIC_H_ */
