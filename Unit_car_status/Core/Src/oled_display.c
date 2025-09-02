/**
 * @file    oled_display.c
 * @brief   SSD1306 OLED 디스플레이에 상태 정보를 출력하는 기능을 구현한다.
 * @author  YeonsuJ
 * @date    2025-07-29
 */

#include "oled_display.h"
#include "ssd1306.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief   OLED 디스플레이를 초기화한다.
 * @note    화면을 지우고 초기 상태를 표시할 준비를 한다.
 */
void OLED_Init(void)
{
    SSD1306_Init();
    SSD1306_Clear();
    SSD1306_UpdateScreen();
}

/**
 * @brief   통신 및 배터리 상태에 따라 OLED 화면 전체를 업데이트한다.
 * @param   percent   배터리 잔량 (단위: %)
 * @param   vout      필터링된 배터리 전압 (단위: V)
 * @param   is_can_ok 통합 CAN 통신 상태 (true: 정상, false: 실패)
 * @param   is_rf_ok  RF 통신 상태 (true: 정상, false: 실패)
 */
void OLED_UpdateDisplay(float percent, float vout, bool is_can_ok, bool is_rf_ok)
{
    // 화면을 업데이트하기 전에 이전 내용을 모두 지운다.
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    // 1. CAN과 RF 통신이 모두 실패한 경우, 두 통신의 실패 메시지를 표시한다.
    if (!is_can_ok && !is_rf_ok)
    {
        SSD1306_GotoXY(23, 12);
        SSD1306_Puts("CAN FAIL", &Font_11x18, 1);
        SSD1306_GotoXY(28, 35);
        SSD1306_Puts("RF FAIL", &Font_11x18, 1);
    }
    // 2. CAN 통신만 실패한 경우, CAN 실패 메시지를 화면 중앙에 표시한다.
    else if (!is_can_ok)
    {
        SSD1306_GotoXY(23, 23);
        SSD1306_Puts("CAN FAIL", &Font_11x18, 1);
    }
    // 3. RF 통신만 실패한 경우, RF 실패 메시지를 화면 중앙에 표시한다.
    else if (!is_rf_ok)
    {
        SSD1306_GotoXY(28, 23);
        SSD1306_Puts("RF FAIL", &Font_11x18, 1);
    }
    // 4. 모든 통신이 정상인 경우, 배터리 정보를 표시한다.
    else
	{
		char line1_str[21];
		char line2_str[21];

		// 첫 번째 라인 문자열 생성: "BAT: 80%" 형태
        // float를 반올림하여 정수형 퍼센트로 표시한다.
		snprintf(line1_str, sizeof(line1_str), "BAT: %3d%%", (uint8_t)(percent + 0.5f));

		// 두 번째 라인 문자열 생성: "(3.12V)" 형태
		snprintf(line2_str, sizeof(line2_str), "(%.2fV)", vout);

		// 화면의 적절한 위치에 두 줄로 출력한다.
		SSD1306_GotoXY(14, 14); // 첫 번째 줄 Y좌표
		SSD1306_Puts(line1_str, &Font_11x18, 1);

		SSD1306_GotoXY(25, 35); // 두 번째 줄 Y좌표
		SSD1306_Puts(line2_str, &Font_11x18, 1);
	}

    // 내부 버퍼에 그려진 내용을 실제 화면으로 전송하여 표시한다.
    SSD1306_UpdateScreen();
}
