/**
 * @file motor_encoder.c
 * @brief DC 모터 엔코더를 이용한 RPM 측정 소스 파일이다.
 * @author YeonsuJ
 * @date 2025-07-25
 */

#include <math.h>
#include "motor_encoder.h"
#include "tim.h"

// --- 상수 정의 ---
#define PPR 8                    // 모터 엔코더의 한 회전당 펄스 수 (Pulse Per Revolution)
#define GEAR_RATIO 21.3f         // 감속 기어비
#define TICKS_PER_REV (PPR * GEAR_RATIO * 4) // 한 바퀴 회전당 발생하는 총 틱 수 (4배수 모드 사용)
#define RPM_FILTER_ALPHA 0.3f    // 저주파 통과 필터 계수 (0.0 ~ 1.0). 값이 클수록 반응이 빠르다.

// --- 타이머 핸들 ---
extern TIM_HandleTypeDef htim1; // DC 모터 엔코더 입력을 위한 타이머 핸들

// --- static 변수 ---
static float motor_rpm = 0.0f;     // 필터링된 최종 RPM 값을 저장하는 변수
static float filtered_rpm = 0.0f;  // 저주파 통과 필터의 내부 계산을 위한 변수

/**
 * @brief DWT Cycle Counter를 활성화하여 초정밀 타이머로 사용한다.
 * @note 이 함수는 HAL_Init() 이후, RTOS 스케줄러 시작 전에 한 번만 호출해야 한다.
 */
void DWT_Init(void)
{
    // DWT 기능 활성화
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // Cycle Counter 활성화
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    // Cycle Counter 리셋
    DWT->CYCCNT = 0;
}

/**
 * @brief 주기적으로 호출되어 엔코더 카운트 변화량과 경과 시간을 바탕으로 RPM을 계산한다.
 * @note 저주파 통과 필터(IIR)를 적용하여 RPM 값을 부드럽게 처리한다.
 */
void Update_Motor_RPM(void)
{
    static int16_t last_encoder_count = 0;   // 이전 엔코더 카운터 값
    static uint32_t last_cycle_count = 0;    // 이전 DWT 사이클 카운트 값

    uint32_t current_cycle_count = DWT->CYCCNT;
    int16_t current_encoder_count = (int16_t)__HAL_TIM_GET_COUNTER(&htim1);

    // 경과 시간 및 엔코더 변화량 계산
    uint32_t delta_cycles = current_cycle_count - last_cycle_count;
    int16_t delta_encoder = current_encoder_count - last_encoder_count;

    // 다음 측정을 위해 현재 값 저장
    last_encoder_count = current_encoder_count;
    last_cycle_count = current_cycle_count;

    // 경과 시간(초) 계산
    float delta_time_s = (float)delta_cycles / (float)SystemCoreClock;

    if (delta_time_s > 0)
    {
        // 1. 실제 경과 시간을 이용해 정확한 RPM 계산
        float pps = (float)delta_encoder / delta_time_s; // Pulses Per Second
        float raw_rpm = (pps / TICKS_PER_REV) * 60.0f;

        // 2. 저주파 통과 필터(IIR) 적용하여 값을 부드럽게 만듦
        filtered_rpm = (RPM_FILTER_ALPHA * raw_rpm) + ((1.0f - RPM_FILTER_ALPHA) * filtered_rpm);
        motor_rpm = filtered_rpm;
    }
}

/**
 * @brief 현재 계산된 모터의 RPM 값을 반환한다.
 * @retval float 필터링된 현재 RPM 값 (항상 양수)
 * @note fabsf() 함수를 사용하여 모터의 회전 방향과 관계없이 항상 양수 값을 반환한다.
 */
float MotorControl_GetRPM(void)
{
	return fabsf(motor_rpm);
}
