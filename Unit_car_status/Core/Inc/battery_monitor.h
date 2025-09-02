/**
 * @file    battery_monitor.h
 * @brief   배터리 모니터링 관련 함수의 선언을 포함한다.
 * @author  YeonsuJ
 * @date    2025-07-28
 */

#ifndef INC_BATTERY_MONITOR_H_
#define INC_BATTERY_MONITOR_H_

#include "main.h"

/**
 * @brief   ADC를 통해 배터리 전압을 읽고, 처리 과정을 거쳐 백분율로 변환한다.
 * @param   vout_ret 필터링이 완료된 ADC 출력 전압(Vout)을 저장할 포인터
 * @retval  계산된 배터리 잔량 (0.0% ~ 100.0%)
 */
float Read_Battery_Percentage(float* vout_ret);

/**
 * @brief   새로운 샘플을 받아 이동 평균 필터를 적용한 값을 반환한다.
 * @param   new_sample 새로운 ADC 측정 전압 샘플
 * @retval  이동 평균 필터가 적용된 전압 값
 */
float Get_Averaged_Vout(float new_sample);

#endif /* INC_BATTERY_MONITOR_H_ */
