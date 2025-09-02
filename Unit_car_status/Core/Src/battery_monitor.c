/**
 * @file    battery_monitor.c
 * @brief   배터리 전압을 ADC로 측정하고, 필터링 및 변환을 통해 배터리 잔량을 백분율로 제공한다.
 * @author  YeonsuJ
 * @date    2025-07-27
 */

#include "battery_monitor.h"
#include "adc.h"

// 이동 평균 필터의 샘플 개수
#define FILTER_SIZE 10
// Vout 측정값에 대한 보정 계수. 하드웨어적인 오차를 보상하기 위해 사용한다.
#define VOUT_CORRECTION_FACTOR 1.010f

// 이동 평균 필터의 데이터 버퍼. 최근 FILTER_SIZE 개의 ADC 출력 전압(Vout)을 저장한다.
float vout_buffer[FILTER_SIZE] = {0};
// vout_buffer 배열의 현재 인덱스를 가리킨다.
uint8_t filter_index = 0;
// 유효한 샘플의 개수를 저장한다. 필터 버퍼가 가득 찰 때까지 증가한다.
uint8_t valid_sample_count = 0;

/**
 * @brief   새로운 샘플을 받아 이동 평균 필터를 적용한 값을 반환한다.
 * @note    내부적으로 순환 버퍼를 사용하여 최근 N개의 데이터에 대한 평균을 계산한다.
 * @param   new_sample 새로운 ADC 측정 전압 샘플
 * @retval  이동 평균 필터가 적용된 전압 값
 */
float Get_Averaged_Vout(float new_sample)
{
    // 새 샘플을 버퍼에 저장하고 인덱스를 업데이트한다.
    vout_buffer[filter_index] = new_sample;
    filter_index = (filter_index + 1) % FILTER_SIZE;

    // 버퍼가 가득 차지 않았다면 유효 샘플 수를 증가시킨다.
    if (valid_sample_count < FILTER_SIZE)
        valid_sample_count++;

    // 현재까지 수집된 유효한 샘플들의 합을 계산한다.
    float sum = 0.0f;
    for (int i = 0; i < valid_sample_count; i++)
        sum += vout_buffer[i];

    // 평균을 계산하여 반환한다.
    return sum / valid_sample_count;
}

/**
 * @brief   ADC를 통해 배터리 전압을 읽고, 처리 과정을 거쳐 백분율로 변환한다.
 * @note    ADC 변환, 보정, 필터링, 전압 복원, 백분율 변환 및 범위 제한의 단계를 거친다.
 * @param   vout_ret 필터링이 완료된 ADC 출력 전압(Vout)을 저장할 포인터. NULL일 경우 무시된다.
 * @retval  계산된 배터리 잔량 (0.0% ~ 100.0%)
 */
float Read_Battery_Percentage(float* vout_ret)
{
    // ADC 변환 시작 및 완료 대기
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint16_t adc_val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    // 1. ADC 디지털 값(0-4095)을 아날로그 전압(Vout_raw)으로 변환한다.
    float vout_raw = (3.3f * adc_val) / 4095.0f;

    // 2. 하드웨어 오차 보정을 위한 고정 보정 계수를 적용한다.
    float vout = vout_raw * VOUT_CORRECTION_FACTOR;

    // 3. 노이즈 제거를 위해 이동 평균 필터를 적용한다.
    vout = Get_Averaged_Vout(vout);

    // 4. 전압 분배 회로의 비율에 따라 실제 배터리 전압(Vbat)을 복원한다.
    //    (예: 실제 Vbat 11.46V가 ADC 입력 Vout 2.92V로 측정된 경우를 기반으로 비율 계산)
    float vbat = vout * (11.46f / 2.92f);

    // 5. Vbat을 배터리 잔량(%)으로 환산한다. (3S Li-Po 배터리 기준: 최소 9.6V, 최대 12.6V)
    float percent = ((vbat - 9.6f) / (12.6f - 9.6f)) * 100.0f;

    // 6. 계산된 백분율 값이 0% 미만이거나 100%를 초과하지 않도록 범위를 제한한다.
    if (percent > 100.0f) percent = 100.0f;
    if (percent < 0.0f) percent = 0.0f;

    // vout_ret 포인터가 유효한 경우, 최종 Vout 값을 저장한다.
    if (vout_ret != NULL)
        *vout_ret = vout;

    return percent;
}
