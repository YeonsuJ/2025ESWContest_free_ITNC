/**
 * @file    led_control.h
 * @brief   LED 제어 관련 함수의 선언을 포함한다.
 * @author  YeonsuJ
 * @date    2025-07-28
 */

#ifndef INC_LED_CONTROL_H_
#define INC_LED_CONTROL_H_

#include "main.h"

/**
 * @brief   외부 상태 값에 따라 LED 점등 상태를 업데이트한다.
 * @param   ldr_dark  조도 센서 기반 어두움 여부 (1: 어두움, 0: 밝음)
 * @param   direction 주행 방향 (1: 전진, 0: 후진)
 * @param   brake     브레이크 상태 (0 초과: ON, 0: OFF)
 */
void LEDControl_Update(uint8_t ldr_dark, uint8_t direction, uint8_t brake);

#endif /* INC_LED_CONTROL_H_ */
