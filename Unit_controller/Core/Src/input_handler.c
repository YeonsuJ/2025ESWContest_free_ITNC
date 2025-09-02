// input_handler.c

#include "input_handler.h"

// === 내부 전용 변수들 (static으로 선언하여 외부 접근 차단) ===
static volatile uint8_t accel_pressed = 0;
static volatile uint8_t brake_pressed = 0;
static volatile uint32_t accel_count = 0; // 20ms 단위 카운트
static volatile uint32_t brake_count = 0; // 20ms 단위 카운트

// === 함수 구현 ===

void InputHandler_Init(void)
{
    // 변수 초기화
    accel_pressed = 0;
    brake_pressed = 0;
    accel_count = 0;
    brake_count = 0;
}

void InputHandler_GpioCallback(uint16_t GPIO_Pin)
{
    // Accel 버튼 (PB0)
    if (GPIO_Pin == GPIO_PIN_0)
    {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) // 버튼 눌림
        {
            accel_pressed = 1;
            accel_count = 0; // 눌리는 순간 카운트 초기화
        }
        else // 버튼 떼짐
        {
            accel_pressed = 0;
            accel_count = 0; // 떼는 순간 카운트 초기화
        }
    }
    // Brake 버튼 (PB1)
    else if (GPIO_Pin == GPIO_PIN_1)
    {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) // 버튼 눌림
        {
            brake_pressed = 1;
            brake_count = 0;
        }
        else // 버튼 떼짐
        {
            brake_pressed = 0;
            brake_count = 0;
        }
    }
}

void InputHandler_TimerCallback(void)
{
    // 버튼이 눌려있는 동안 카운트 증가
    if (accel_pressed)
    {
        accel_count++;
    }
    if (brake_pressed)
    {
        brake_count++;
    }
}

uint16_t InputHandler_GetAccelMillis(void)
{
    // Race Condition을 피하기 위해 잠시 인터럽트를 비활성화하고 값을 읽음
    uint32_t count_snapshot;
    __disable_irq();
    count_snapshot = accel_count;
    __enable_irq();

    return (uint16_t)(count_snapshot * 20);
}

uint16_t InputHandler_GetBrakeMillis(void)
{
    // Race Condition을 피하기 위해 잠시 인터럽트를 비활성화하고 값을 읽음
    uint32_t count_snapshot;
    __disable_irq();
    count_snapshot = brake_count;
    __enable_irq();

    return (uint16_t)(count_snapshot * 20);
}

LockerDirection InputHandler_GetDirection(void)
{
    // 락커 스위치 판별 (PB9: FORWARD)
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_RESET)
        return LOCKER_FORWARD;
    else
        return LOCKER_BACKWARD;
}
