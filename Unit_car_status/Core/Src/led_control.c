/**
 * @file    led_control.c
 * @brief   차량의 LED(헤드/리어라이트, 브레이크등)를 상태에 따라 제어하는 기능을 구현한다.
 * @author  YeonsuJ
 * @date    2025-07-28
 */

#include "led_control.h"

/**
 * @brief   입력된 상태에 따라 모든 LED의 점등 상태를 업데이트한다.
 * @note    이 함수가 호출될 때마다 먼저 모든 LED를 끈 후, 조건에 맞는 LED만 다시 켠다.
 * @param   ldr       조도 센서 상태. (1: 어두움, 0: 밝음)
 * @param   direction 주행 방향 상태. (1: 전진, 0: 후진)
 * @param   brake     브레이크 상태. (0 초과: 브레이크 ON, 0: 브레이크 OFF)
 */
void LEDControl_Update(uint8_t ldr, uint8_t direction, uint8_t brake)
{
    // 1. 상태를 업데이트하기 전 모든 LED를 초기화(OFF)한다.
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);

    // 2. 전조등 제어: 어두울 때만 방향에 따라 점등된다.
    if (ldr == 1)  // 어두운 상태일 경우
    {
        if (direction == 1) // 전진
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
        }
        else if (direction == 0) // 후진
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
        }
    }

    // 3. 브레이크등 제어: 조도 상태와 관계없이 브레이크가 활성화되면 항상 점등된다.
    if (brake > 0)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
    }
}
