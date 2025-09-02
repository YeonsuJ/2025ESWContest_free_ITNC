/**
 * @file can_handler.c
 * @brief CAN 통신 수신 및 송신 로직을 구현한다.
 * @author YeonsuJ
 * @date 2025-07-23
 * @note FreeRTOS 메시지 큐를 사용하여 CAN 수신 데이터를 비동기적으로 처리한다.
 */
#include "can_handler.h"
#include "can.h"
#include "cmsis_os.h"
#include <stdbool.h>

/**
 * @brief CAN 수신(Rx) 패킷 구조 상세 정의
 * @details
 * - **수신 ID**:
 * - StdId: 0x6A5 (슬레이브 -> 마스터)
 * - 필터 모드: 11비트 표준 ID, 마스크 0x7FF (모든 비트 일치)
 * - **수신 헤더 (RxHeader)**:
 * - `RxHeader.StdId`: 송신 측 ID (0x6A5가 맞는지 확인)
 * - `RxHeader.DLC`: 데이터 길이 (최소 4바이트 이상이어야 함)
 * - **수신 데이터 (RxData)**:
 * - `RxData[0]`: 거리 조건 결과 (1,2,3: 위험 -> 1로 변환 / 그 외: 안전 -> 0으로 변환)
 * - `RxData[1]`: 예비, 현재 미사용 (조도센서 데이터는 central에서 수신하지 않음)
 * - `RxData[2]`: 모터 RPM 하위 바이트(LSB)
 * - `RxData[3]`: 모터 RPM 상위 바이트(MSB)
 * - `RxData[4]~RxData[7]`: 예비, 현재 미사용
 */

/**
 * @brief CAN 수신 데이터를 처리할 태스크로 전달하기 위한 FreeRTOS 메시지 큐 핸들.
 * @note 이 큐는 `freertos.c`에서 생성되며, 수신 콜백 함수에서 데이터를 `put`한다.
 */
extern osMessageQueueId_t CANRxQueueHandle;

CAN_FilterTypeDef sFilterConfig;    // CAN 필터 설정 구조체
CAN_RxHeaderTypeDef RxHeader;       // 수신 메시지 헤더 저장용 변수
uint8_t RxData[8];                  // 수신 메시지 데이터 저장용 버퍼

/**
 * @brief CAN 컨트롤러를 시작하고 수신 필터를 설정한다.
 */
void CANHandler_Init(void)
{
	extern CAN_HandleTypeDef hcan;
	HAL_CAN_Start(&hcan);
	CAN_Filter_Config(&hcan);
}

/**
 * @brief CAN 메시지 수신 필터를 설정하고 인터럽트를 활성화한다.
 * @param hcan_ptr CAN 핸들러 포인터
 * @note 특정 CAN ID(0x6A5)를 가진 메시지만 수신하도록 필터를 구성한다.
 * 메시지가 FIFO1으로 수신되도록 설정하고, 관련 인터럽트를 활성화한다.
 */
void CAN_Filter_Config(CAN_HandleTypeDef *hcan_ptr)
{
   // Configure the filter
   sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
   sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
   sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
   sFilterConfig.FilterIdHigh = 0x6A5<<5;
   sFilterConfig.FilterIdLow = 0;
   sFilterConfig.FilterMaskIdHigh = 0x7FF<<5; // 모든 비트가 일치해야 함
   sFilterConfig.FilterMaskIdLow = 0;
   sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;

   if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
           Error_Handler(); // 실패 시 에러 처리

   // 수신 인터럽트 활성화
   if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING) != HAL_OK)
           Error_Handler(); // 실패 시 에러 처리
}

/**
 * @brief CAN FIFO1에 메시지 수신이 보류 중일 때 호출되는 인터럽트 콜백 함수
 * @param hcan CAN 핸들러 포인터
 * @note 이 함수는 ISR 컨텍스트에서 실행된다.
 * 수신된 메시지의 ID와 데이터 길이(DLC)를 확인한 후, 데이터를 파싱하여
 * `CAN_RxPacket_t` 구조체에 담아 FreeRTOS 메시지 큐로 전송한다.
 */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData);

  // ID가 0x6A5이고 데이터 길이가 4바이트 이상인지 확인
  if (RxHeader.StdId == 0x6A5 && RxHeader.DLC >= 4)
  {
	  // 1. 큐로 보낼 데이터를 담을 구조체 변수 선언
	  CAN_RxPacket_t rx_packet;

	  // 2. 수신한 데이터를 파싱하여 구조체에 복사
	  // RxData[0] 값이 1, 2, 3 중 하나일 때만 위험(1)으로, 그 외에는 안전(0)으로 변환
	  rx_packet.distance_signal = (RxData[0] >= 1 && RxData[0] <= 3) ? 1 : 0;

	  // 2바이트(MSB, LSB)를 조합하여 16비트 RPM 값으로 복원
	  // 예: RxData[3]=0x01, RxData[2]=0x2C -> (0x01 << 8) | 0x2C -> 0x012C (300)
	  rx_packet.motor_rpm = (uint16_t)(RxData[3] << 8) | RxData[2];

      // 3. 구조체 변수의 주소를 큐로 전송한다.
      // 이 함수 호출 하나로 데이터 전송과 CAN 처리 태스크를 깨우는 작업이 모두 완료된다.
      osMessageQueuePut(CANRxQueueHandle, &rx_packet, 0U, 0U);
  }
}

/**
 * @brief 차량의 현재 주행 상태를 CAN 버스를 통해 전송한다.
 * @param direction 현재 주행 방향 (0: 후진, 1: 전진)
 * @param brake_status 브레이크 상태 (1: 활성, 0: 비활성)
 * @param rf_status RF 수신 상태 (1: 정상, 0: 끊김)
 * @note CAN ID 0x321을 사용하여 3바이트의 데이터를 전송한다.
 * 슬레이브(센서) 측에서 이 ID를 수신하도록 필터 설정이 필요하다.
 */
void CAN_Send_DriveStatus(uint8_t direction, uint8_t brake_status, uint8_t rf_status)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[3];
    uint32_t TxMailbox;

    TxHeader.StdId = 0x321;  // 송신 ID
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 3;       // 데이터 길이
    TxHeader.TransmitGlobalTime = DISABLE;

    TxData[0] = direction;
    TxData[1] = brake_status;
    TxData[2] = rf_status;

    HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);
}

