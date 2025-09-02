/**
 * @file can_handler.h
 * @brief CAN 통신을 위한 초기화 및 데이터 처리 함수 프로토타입을 정의한다.
 * @author YeonsuJ
 * @date 2025-07-23
 */
#ifndef INC_CAN_HANDLER_H_
#define INC_CAN_HANDLER_H_

#include "main.h"

/**
 * @brief CAN 수신 메시지 상세 설명
 * @details
 * - **ID**: 0x6A5
 * - **RxData[0]**: 거리 조건 (1: 위험, 0: 안전)
 * - **RxData[2]**: 모터 RPM 하위 바이트 (LSB)
 * - **RxData[3]**: 모터 RPM 상위 바이트 (MSB)
 */

 /**
 * @brief CAN으로 수신된 패킷 데이터를 저장하기 위한 구조체
 * @note 콜백 함수에서 이 구조체에 데이터를 채워 메시지 큐로 전송한다.
 */
typedef struct {
    uint8_t distance_signal; ///< 거리 센서 신호 (RxData[0] 기반). 위험 시 1, 안전 시 0
    uint16_t motor_rpm;      ///< 모터 RPM 값 (RxData[2]와 RxData[3]를 조합)
} CAN_RxPacket_t;

// can 수신을 위한 필터 및 수신 버퍼 구성
extern CAN_FilterTypeDef sFilterConfig; // CAN 필터 설정 구조체
extern CAN_RxHeaderTypeDef RxHeader;    // 수신된 CAN 메시지의 헤더 정보
extern uint8_t RxData[8];               // 수신된 CAN 메시지의 데이터 페이로드

/**
 * @brief CAN 통신을 초기화하고 필터를 설정한다.
 */
void CANHandler_Init(void);

/**
 * @brief CAN 수신 필터를 설정한다.
 * @param hcan_ptr CAN 핸들러 포인터
 * @note ID 0x6A5만 수신하도록 필터를 구성한다.
 */
void CAN_Filter_Config(CAN_HandleTypeDef *hcan_ptr);

/**
 * @brief CAN FIFO1 메시지 수신 보류 콜백 함수
 * @param hcan CAN 핸들러 포인터
 * @note CAN 메시지가 수신되면 HAL 드라이버에 의해 자동으로 호출된다.
 */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan);

/**
 * @brief 차량의 주행 상태(방향, 브레이크, RF 상태)를 CAN 버스로 전송한다.
 * @param direction 현재 주행 방향 (0: 후진, 1: 전진)
 * @param brake_status 브레이크 상태 (1: 활성, 0: 비활성)
 * @param rf_status RF 수신 상태 (1: 정상, 0: 끊김)
 */
void CAN_Send_DriveStatus(uint8_t direction, uint8_t brake_status, uint8_t rf_status);

#endif /* INC_CAN_HANDLER_H_ */
