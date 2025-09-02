/**
 * @file    can_handler.h
 * @brief   CAN 통신 처리 관련 자료구조 및 함수 선언을 포함한다.
 * @author  YeonsuJ
 * @date    2025-07-28
 */

#ifndef __CAN_HANDLER_H
#define __CAN_HANDLER_H

#include "main.h"
#include <stdbool.h>
#include "cmsis_os.h"

/**
 * @brief CAN 수신 메시지를 FreeRTOS 큐로 전달하기 위한 패킷 구조체
 */
typedef struct {
    CAN_RxHeaderTypeDef header; // CAN 메시지 헤더 (ID, 길이 등)
    uint8_t data[8];            // 수신된 데이터 (최대 8바이트)
} CAN_RxPacket_t;

// FreeRTOS 객체 및 공유 변수에 대한 extern 선언
extern osMessageQueueId_t CANRxQueueHandle;     // CAN 수신 메시지를 담는 큐
extern osMutexId_t SharedDataMutexHandle;       // 공유 데이터 접근을 위한 뮤텍스

// CAN 노드별 타임아웃 감지를 위한 타임스탬프 변수
extern volatile uint32_t g_last_rx_time_central; // Central 보드로부터 마지막 메시지를 수신한 시간
extern volatile uint32_t g_last_rx_time_sensor; // Sensor 보드로부터 마지막 메시지를 수신한 시간

/**
 * @brief CAN 통신을 초기화한다.
 */
void CANHandler_Init(void);

/**
 * @brief CAN 수신 필터를 설정한다.
 * @param hcan_ptr CAN 핸들러에 대한 포인터
 */
void CAN_Filter_Config(CAN_HandleTypeDef *hcan_ptr);

#endif /* __CAN_HANDLER_H */
