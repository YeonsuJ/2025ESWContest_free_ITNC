/**
 * @file    can_handler.c
 * @brief   CAN 통신 초기화, 필터 설정 및 수신 인터럽트 처리를 담당한다.
 * @author  YeonsuJ
 * @date    2025-07-28
 * @note    수신된 CAN 메시지는 FreeRTOS 메시지 큐를 통해 별도의 태스크로 전달되어 처리된다.
 */

#include "can_handler.h"
#include "can.h"
#include "cmsis_os.h"

/**
 * @note CAN 수신 패킷의 ID 및 데이터 구조
 * - ID 0x6A5 (Sensor Board):
 * - data[1]: LDR 센서 값 기반의 어두움 여부 (1: dark, 0: bright)
 * - ID 0x321 (Central Board):
 * - data[0]: 주행 방향 (1: forward, 0: backward)
 * - data[1]: 브레이크 상태 (1: on, 0: off)
 */

// CAN 수신 필터 설정을 위한 구조체 변수
CAN_FilterTypeDef sFilterConfig;

/**
 * @brief   CAN 컨트롤러를 시작하고 수신 필터를 설정하며, 수신 인터럽트를 활성화한다.
 * @note    이 함수는 Main 초기화 과정에서 한 번만 호출되어야 한다.
 */
void CANHandler_Init(void)
{
    extern CAN_HandleTypeDef hcan;
    HAL_CAN_Start(&hcan);
    CAN_Filter_Config(&hcan);

    // FIFO1에 메시지가 수신되면 인터럽트가 발생하도록 활성화한다.
    if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING) != HAL_OK)
        Error_Handler();
}

/**
 * @brief   CAN 메시지 수신 필터를 설정한다.
 * @note    현재 설정은 모든 CAN ID를 수신하도록 구성되어 있다.
 * @param   hcan_ptr CAN 핸들러에 대한 포인터
 */
void CAN_Filter_Config(CAN_HandleTypeDef *hcan_ptr)
{
	// 필터를 활성화하고, 수신된 메시지는 FIFO1에 저장하도록 설정한다.
	sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
	sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
	// Identifier Mask 모드로 설정하여 특정 비트만 비교한다.
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;

	// ID와 Mask를 모두 0으로 설정하여 모든 ID의 메시지를 수신하도록 한다.
	// (센서 보드의 0x6A5, 중앙 보드의 0x321 메시지를 모두 받기 위함)
	sFilterConfig.FilterIdHigh     = 0x0000;
	sFilterConfig.FilterIdLow      = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;
	sFilterConfig.FilterMaskIdLow  = 0x0000;

	// 설정된 필터를 CAN 컨트롤러에 적용한다.
	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
	        Error_Handler(); // 실패 시 에러 처리
}

/**
  * @brief  CAN RX FIFO1에 메시지가 수신되었을 때 호출되는 인터럽트 콜백 함수
  * @note   ISR(Interrupt Service Routine)에서는 최소한의 작업만 수행해야 한다.
  * 수신된 메시지를 FreeRTOS 메시지 큐에 넣어 실제 처리는 CAN 처리 태스크에서 수행하도록 한다.
  * @param  hcan CAN handle
  * @retval None
  */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxPacket_t rxPacket;

    // CAN 하드웨어 수신 버퍼(FIFO1)에서 메시지를 읽어온다.
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &rxPacket.header, rxPacket.data) == HAL_OK)
    {
        // 메시지 큐가 생성되었다면, 읽어온 메시지를 큐에 전송한다.
        if (CANRxQueueHandle != NULL)
        {
        	// ISR에서는 대기 시간을 0으로 설정하여 블로킹되지 않도록 한다.
        	osMessageQueuePut(CANRxQueueHandle, &rxPacket, 0U, 0U);
        }
    }
}
