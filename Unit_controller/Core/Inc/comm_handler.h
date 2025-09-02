// comm_handler.h

#ifndef INC_COMM_HANDLER_H_
#define INC_COMM_HANDLER_H_

#include "main.h"

// ACK 페이로드 크기 정의
#define ACK_PAYLOAD_SIZE 3
#define PAYLOAD_SIZE 8

// 송신 결과 상태를 나타내는 열거형
typedef enum {
    COMM_OK,          // 별다른 이벤트 없음
    COMM_TX_SUCCESS,  // 송신 성공 및 ACK 수신
    COMM_TX_FAIL      // 송신 실패 (MAX_RT)
} CommStatus_t;

void CommHandler_Init(void);
void CommHandler_IrqCallback(void);
void CommHandler_Transmit(uint8_t* payload, uint8_t len);
CommStatus_t CommHandler_CheckStatus(uint8_t* ack_payload, uint8_t len);

#endif /* INC_COMM_HANDLER_H_ */
