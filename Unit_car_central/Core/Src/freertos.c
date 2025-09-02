/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h> // memcpy를 사용하기 위해 필요
#include "motor_control.h"
#include "can_handler.h"
#include "rf_handler.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RF_SEMAPHORE_TIMEOUT 250 // 250ms 동안 RF 신호가 없으면 실패로 간주
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for RFTask */
osThreadId_t RFTaskHandle;
const osThreadAttr_t RFTask_attributes = {
  .name = "RFTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for CANTask */
osThreadId_t CANTaskHandle;
const osThreadAttr_t CANTask_attributes = {
  .name = "CANTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CANTxQueue */
osMessageQueueId_t CANTxQueueHandle;
const osMessageQueueAttr_t CANTxQueue_attributes = {
  .name = "CANTxQueue"
};
/* Definitions for CANRxQueue */
osMessageQueueId_t CANRxQueueHandle;
const osMessageQueueAttr_t CANRxQueue_attributes = {
  .name = "CANRxQueue"
};
/* Definitions for RFSem */
osSemaphoreId_t RFSemHandle;
const osSemaphoreAttr_t RFSem_attributes = {
  .name = "RFSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartRFTask(void *argument);
void StartCANTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of RFSem */
  RFSemHandle = osSemaphoreNew(1, 0, &RFSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of CANTxQueue */
  CANTxQueueHandle = osMessageQueueNew (2, sizeof(VehicleCommand_t), &CANTxQueue_attributes);

  /* creation of CANRxQueue */
  CANRxQueueHandle = osMessageQueueNew (1, sizeof(CAN_RxPacket_t), &CANRxQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of RFTask */
  RFTaskHandle = osThreadNew(StartRFTask, NULL, &RFTask_attributes);

  /* creation of CANTask */
  CANTaskHandle = osThreadNew(StartCANTask, NULL, &CANTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartRFTask */
/**
* @brief RF 통신 수신 및 주행 제어를 총괄하는 최상위 태스크
* @param argument: None
* @note 이 태스크는 다음과 같은 순서로 동작한다:
* 1. RF 수신 인터럽트(세마포어)를 타임아웃과 함께 대기한다.
* 2. 수신 성공 시, CANTask로부터 받은 최신 CAN 데이터(거리, RPM)가 있는지 확인하고, 있다면 ACK 페이로드에 반영할 준비를 한다.
* 3. RF 수신 버퍼의 모든 주행 명령을 `RFHandler_GetNewCommand`를 통해 처리한다.
* 4. 각 명령에 대해 모터를 제어(`MotorControl_Update`)하고, 해당 명령을 CANTask로 전달(`osMessageQueuePut`)한다.
* 5. 다음 전송을 위해 준비된 ACK 페이로드를 설정(`RFHandler_SetAckPayload`)한다.
* 6. 만약 RF 수신이 타임아웃되면, RF 통신이 끊어진 것으로 간주하고 RF 실패 상태를 CANTask로 전송한다.
*/
/* USER CODE END Header_StartRFTask */
void StartRFTask(void *argument)
{
  /* USER CODE BEGIN StartRFTask */
	VehicleCommand_t cmd = {0};

	//큐에서 받을 데이터를 담을 구조체 변수
	CAN_RxPacket_t received_can_packet;

	// RF ACK 페이로드로 보낼 3바이트 데이터 버퍼 선언 및 초기화
	// [0]:햅틱, [1]:RPM(하위), [2]:RPM(상위)
	uint8_t ack_payload[3] = {0};
  /* Infinite loop */
	for(;;)
	  {
	      // RF 수신 인터럽트를 타임아웃과 함께 대기
		if (osSemaphoreAcquire(RFSemHandle, RF_SEMAPHORE_TIMEOUT) == osOK)
		{
	      // CAN 수신 큐에서 최신 거리 값을 논블로킹으로 확인
		  // 성공적으로 새 데이터를 받으면 ack_payload를 업데이트
		  if (osMessageQueueGet(CANRxQueueHandle, &received_can_packet, NULL, 0U) == osOK)
		  {
			  // 큐에서 받은 구조체 데이터로 ack_payload 배열 채우기

			  // ack_payload[0]에는 햅틱을 위한 distance_signal 저장
			  ack_payload[0] = received_can_packet.distance_signal;

			  // memcpy를 사용하여 RPM 값을 페이로드에 저장
			  uint16_t rpm_value = received_can_packet.motor_rpm;
			  // rpm_value 변수의 메모리 내용을 ack_payload[1] 주소에 2바이트(sizeof(uint16_t))만큼 복사합니다.
			  // 이 방식은 시스템의 Endianness(리틀/빅 엔디안)에 따라 자동으로 바이트 순서가 결정된다.
			  memcpy(&ack_payload[1], &rpm_value, sizeof(uint16_t));
		  }
	      // RF 수신 버퍼에 있는 모든 명령을 처리
		  while (RFHandler_GetNewCommand(&cmd)) {

			  // 수신 성공 시, 구조체에 RF 상태(true)를 기록
			  cmd.rf_status = true;

	      // 모터 제어 업데이트
			  MotorControl_Update(&cmd);

	      // CAN 전송을 위해 수신한 cmd 구조체 전체를 CANTxQueue에 넣음
			  osMessageQueuePut(CANTxQueueHandle, &cmd, 0U, 0U);

	      // 컨트롤러에 보낼 ACK 페이로드 설정 (배열과 크기 전달)
			  // GetNewCommand 함수 내부에서 ACK를 보내므로, 이 함수를 호출한 직후에 ACK 페이로드를 설정해야 다음 ACK에 반영됨 (while loop 내부)
	      // (가장 최신 CAN 데이터로 매번 ACK 페이로드를 설정)
			  RFHandler_SetAckPayload(ack_payload, sizeof(ack_payload));
		  }
		}
		else
		{
			// RF 신호 수신 타임아웃 (실패)
			// RF 실패 상태를 CANTask로 알리기 위해 상태 메시지 전송
			memset(&cmd, 0, sizeof(VehicleCommand_t)); // 안전을 위해 주행 명령 초기화
			cmd.rf_status = false; // 구조체에 RF 상태(false) 기록
			osMessageQueuePut(CANTxQueueHandle, &cmd, 0U, 0U); // CANTask로 전송
		}
	  }
  /* USER CODE END StartRFTask */
}

/* USER CODE BEGIN Header_StartCANTask */
/**
* @brief RFTask로부터 받은 주행 상태를 CAN 버스로 전송하는 태스크
* @param argument: None
* @note 이 태스크는 `CANTxQueue`에 데이터가 들어올 때까지 무한정 대기한다.
* `RFTask`가 큐에 `VehicleCommand_t` 구조체를 넣으면, 이 태스크는 깨어나서
* 구조체에서 방향, 브레이크, RF 상태 정보를 추출하여 `CAN_Send_DriveStatus` 함수를 통해 전송한다.
*/
/* USER CODE END Header_StartCANTask */
void StartCANTask(void *argument)
{
  /* USER CODE BEGIN StartCANTask */
	VehicleCommand_t received_cmd;

  /* Infinite loop */
  for(;;)
  {
      // 큐에서 VehicleCommand_t 구조체 수신
	  osMessageQueueGet(CANTxQueueHandle, &received_cmd, NULL, osWaitForever);

      // 수신한 구조체에서 필요한 데이터 추출
      uint8_t dir = received_cmd.direction;
      uint8_t brake = (received_cmd.brake_ms > 0) ? 1 : 0;
      bool rf_ok = received_cmd.rf_status;

      // 실제 CAN 전송 함수 호출
      CAN_Send_DriveStatus(dir, brake, rf_ok);
  }
  /* USER CODE END StartCANTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

