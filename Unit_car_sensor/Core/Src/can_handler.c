/**
 * @file can_handler.c
 * @brief CAN 통신 메시지 전송 처리 소스 파일
 * @author YeonsuJ
 * @date 2025-07-26
 */

#include "can_handler.h"
#include "can.h"
#include <stdbool.h> 

// --- 전역 변수 ---
CAN_TxHeaderTypeDef TxHeader; // CAN 전송 메시지 헤더 구조체
uint32_t TxMailbox;           // CAN 전송 메일박스 번호 저장 변수
uint8_t TxData[8];            // CAN 전송 데이터 버퍼

/**
 * @brief CAN 통신과 Tx 메시지 헤더를 초기화한다.
 */
void CAN_tx_Init(void)
{
	HAL_CAN_Start(&hcan);

	TxHeader.DLC = 4;             // 데이터 길이 코드 (Data Length Code) = 4 Bytes
	TxHeader.IDE = CAN_ID_STD;    // ID 타입 = 표준 ID (Standard ID)
	TxHeader.RTR = CAN_RTR_DATA;  // 프레임 타입 = 데이터 프레임 (Data Frame)
	TxHeader.StdId = 0x6A5;       // 표준 ID 값
}

/**
 * @brief 준비된 CAN TxData를 전송한다.
 * @note 이 함수는 TxData 배열에 전송할 데이터가 채워진 후 호출되어야 한다.
 */
void CAN_Send(void)
{
    // CAN 메시지 전송 함수 호출
    HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);
}
