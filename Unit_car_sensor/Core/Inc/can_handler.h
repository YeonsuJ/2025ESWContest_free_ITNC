/**
 * @file can_handler.h
 * @brief CAN 통신 처리 관련 함수 및 구조체를 선언한다.
 * @author YeonsuJ
 * @date 2025-7-26
 */

#ifndef INC_CAN_TX_HANDLER_H_
#define INC_CAN_TX_HANDLER_H_

#include "main.h"
#include <stdbool.h>

/**
 * @brief SensorTask가 CANTask로 데이터를 전달하기 위한 구조체이다.
 */
typedef struct {
    uint32_t distance_front;  // 전방 초음파 센서 거리 값
    uint32_t distance_rear;   // 후방 초음파 센서 거리 값
    uint8_t  light_condition; // 조도 센서 상태 값
    float    rpm;             // 모터 RPM 값
} SensorData_t;

// --- 외부 전역 변수 ---
extern uint8_t TxData[8]; // 전송될 CAN 데이터 버퍼

/**
 * @brief CAN 통신을 초기화한다.
 */
void CAN_tx_Init(void);

/**
 * @brief CAN 메시지를 전송한다.
 */
void CAN_Send(void);

#endif /* INC_CAN_TX_HANDLER_H_ */
