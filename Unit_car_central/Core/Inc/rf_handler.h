/**
 * @file rf_handler.h
 * @brief NRF24L01+ 무선 통신 모듈을 제어하고 주행 명령을 수신하기 위한 함수와 타입을 정의한다.
 * @author YeonsuJ
 * @date 2025-07-22
 */
#ifndef INC_RF_HANDLER_H_
#define INC_RF_HANDLER_H_

#include "main.h"
#include <stdbool.h>

/**
 * @brief 조종기로부터 수신된 주행 명령 데이터를 담는 구조체
 */
typedef struct {
    float roll;         // 조향 값 (-90.0 ~ 90.0)
    uint16_t accel_ms;  // 가속 버튼 유지 시간 (ms)
    uint16_t brake_ms;  // 브레이크 버튼 유지 시간 (ms)
    uint8_t direction;  // 주행 방향 (0: 후진, 1: 전진)
    bool rf_status;     // RF 수신 상태 (true: 정상, false: 끊김)
} VehicleCommand_t;

/**
 * @brief 통신 모듈을 수신(Rx) 모드로 초기화한다.
 * @note NRF24 모듈의 채널, 데이터 속도, 주소 등을 설정하고 수신 대기 상태로 진입시킨다.
 */
void RFHandler_Init(void);

/**
 * @brief NRF24 IRQ 핀 인터럽트 발생 시 호출될 콜백 함수.
 * @note ISR 컨텍스트에서 실행되며, 데이터 수신을 알리기 위해 FreeRTOS 세마포어를 반환(release)한다.
 */
void RFHandler_IrqCallback(void);

/**
 * @brief 다음에 전송할 ACK 페이로드 데이터를 설정한다.
 * @note 이 함수를 통해 설정된 데이터는 다음 수신 성공 시 조종기 측으로 자동 전송된다.
 * @param payload 전송할 데이터가 담긴 버퍼의 포인터
 * @param length 전송할 데이터의 길이
 */
void RFHandler_SetAckPayload(uint8_t* payload, uint8_t length);

/**
 * @brief 새로운 주행 명령이 수신되었는지 확인하고, 있다면 데이터를 파싱하여 반환한다.
 * @param command 파싱된 명령 데이터를 저장할 구조체의 포인터
 * @retval true 새로운 명령이 수신되어 성공적으로 처리되었음을 의미한다.
 * @retval false 새로운 수신 데이터가 없음을 의미한다.
 */
bool RFHandler_GetNewCommand(VehicleCommand_t* command);

#endif /* INC_RF_HANDLER_H_ */
