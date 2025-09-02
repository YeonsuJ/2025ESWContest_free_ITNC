/**
 * @file    oled_display.h
 * @brief   OLED 디스플레이 제어 관련 함수의 선언을 포함한다.
 * @author  YeonsuJ
 * @date    2025-07-28
 */

#ifndef INC_OLED_DISPLAY_H_
#define INC_OLED_DISPLAY_H_

#include <stdbool.h>
#include "main.h"

/**
 * @brief   OLED 디스플레이를 초기화한다.
 */
void OLED_Init(void);

/**
 * @brief   통신 및 배터리 상태에 따라 OLED 화면 전체를 업데이트한다.
 * @param   percent   배터리 잔량 (단위: %)
 * @param   vout      필터링된 배터리 전압 (단위: V)
 * @param   is_can_ok 통합 CAN 통신 상태 (true: 정상, false: 실패)
 * @param   is_rf_ok  RF 통신 상태 (true: 정상, false: 실패)
 */
void OLED_UpdateDisplay(float percent, float vout, bool is_can_ok, bool is_rf_ok);

#endif /* INC_OLED_DISPLAY_H_ */
