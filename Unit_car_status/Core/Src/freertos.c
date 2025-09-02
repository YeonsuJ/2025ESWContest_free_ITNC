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
#include "can.h"
#include "can_handler.h"
#include "battery_monitor.h"
#include "oled_display.h"
#include "led_control.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/**
 * @brief CANTask에서 처리된 데이터를 DisplayTask로 전달하기 위한 구조체
 */
typedef struct {
    // Sensor 데이터
    uint8_t status_distance;    // 거리 센서 상태
    uint8_t status_ldr;         // LDR 센서 값 (어두움 여부)
    // Central 데이터
    uint8_t status_direction;   // 주행 방향 상태
    uint8_t status_brake;       // 브레이크 상태
    bool rf_ok;                 // Central 보드의 RF 통신 상태
    // CAN 통신 상태 진단 결과
	bool is_central_can_ok;     // Central 보드와의 CAN 통신 정상 여부
	bool is_sensor_can_ok;      // Sensor 보드와의 CAN 통신 정상 여부
} DisplayData_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// CAN 메시지 수신 타임아웃 시간 (ms). 이 시간 동안 메시지가 없으면 통신 두절로 간주한다.
#define CAN_TIMEOUT_MS 250
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern volatile uint32_t g_last_rx_time_central;
extern volatile uint32_t g_last_rx_time_sensor;
extern CAN_HandleTypeDef hcan;
/* USER CODE END Variables */
/* Definitions for CANTask */
osThreadId_t CANTaskHandle;
const osThreadAttr_t CANTask_attributes = {
  .name = "CANTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CANRxQueue */
osMessageQueueId_t CANRxQueueHandle;
const osMessageQueueAttr_t CANRxQueue_attributes = {
  .name = "CANRxQueue"
};
/* Definitions for DisplayDataQueue */
osMessageQueueId_t DisplayDataQueueHandle;
const osMessageQueueAttr_t DisplayDataQueue_attributes = {
  .name = "DisplayDataQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartCANTask(void *argument);
void StartDisplayTask(void *argument);

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

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of CANRxQueue */
  CANRxQueueHandle = osMessageQueueNew (10, sizeof(CAN_RxPacket_t), &CANRxQueue_attributes);

  /* creation of DisplayDataQueue */
  DisplayDataQueueHandle = osMessageQueueNew (5, sizeof(DisplayData_t), &DisplayDataQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of CANTask */
  CANTaskHandle = osThreadNew(StartCANTask, NULL, &CANTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(StartDisplayTask, NULL, &DisplayTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartCANTask */
/**
  * @brief  CANTask: CAN 수신 메시지를 처리하고 통신 상태를 진단한다.
  * @param  argument: None
  * @retval None
  * @details 20ms 주기로 CAN Rx 큐를 확인하여 수신된 모든 메시지를 처리한다.
  * 최신 데이터와 각 노드별 통신 타임아웃을 진단한 결과를 종합하여 DisplayDataQueue를 통해 DisplayTask로 전송한다.
  */
/* USER CODE END Header_StartCANTask */
void StartCANTask(void *argument)
{
  /* USER CODE BEGIN StartCANTask */
    CAN_RxPacket_t rxPacket;
    DisplayData_t displayData = {0};  // DisplayTask로 전송할 데이터 구조체

    CANHandler_Init(); // CAN 컨트롤러 시작, 필터 설정 및 수신 인터럽트 활성화

    for(;;)
	{
		// 20ms 주기로 동작하여 시스템의 다른 태스크에 영향을 주지 않으면서도 충분한 반응성을 확보한다.
		osDelay(20);

		// 1. CAN 수신 큐에 쌓여있는 모든 메시지를 non-blocking 방식으로 읽어들여 데이터를 최신 상태로 업데이트한다.
		while (osMessageQueueGet(CANRxQueueHandle, &rxPacket, NULL, 0) == osOK)
		{
			if (rxPacket.header.StdId == 0x6A5) // Sensor 보드로부터의 메시지인 경우
			{
				g_last_rx_time_sensor = HAL_GetTick(); // 마지막 수신 시간 갱신
				displayData.status_ldr = rxPacket.data[1];
			}
			else if (rxPacket.header.StdId == 0x321) // Central 보드로부터의 메시지인 경우
			{
				g_last_rx_time_central = HAL_GetTick(); // 마지막 수신 시간 갱신
				displayData.status_direction = rxPacket.data[0];
				displayData.status_brake = rxPacket.data[1];
				displayData.rf_ok = (bool)rxPacket.data[2];
			}
		}

		// 2. CAN 통신 상태를 진단한다.
		uint32_t now = HAL_GetTick();
		// CAN 컨트롤러 하드웨어 자체의 에러 상태를 확인한다.
		bool is_status_hw_ok = (HAL_CAN_GetState(&hcan) != HAL_CAN_STATE_ERROR);

		// Central 노드와의 통신 상태: HW 정상이고, 마지막 수신 후 타임아웃이 지나지 않았는지 확인한다.
		displayData.is_central_can_ok = is_status_hw_ok && ((now - g_last_rx_time_central) < CAN_TIMEOUT_MS);
		// Sensor 노드와의 통신 상태: HW 정상이고, 마지막 수신 후 타임아웃이 지나지 않았는지 확인한다.
		displayData.is_sensor_can_ok = is_status_hw_ok && ((now - g_last_rx_time_sensor) < CAN_TIMEOUT_MS);

		// 3. 처리된 최신 데이터 패키지를 DisplayDataQueue로 전송하여 DisplayTask가 사용하도록 한다.
		osMessageQueuePut(DisplayDataQueueHandle, &displayData, 0, 0);
	}
  /* USER CODE END StartCANTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief DisplayTask: LED 및 OLED 디스플레이를 업데이트한다.
* @param argument: None
* @retval None
* @details DisplayDataQueue로부터 새로운 데이터가 수신될 때까지 대기한다.
* 데이터 수신 시, LED 상태는 즉시 업데이트하고 OLED 화면은 과도한 업데이트를 방지하기 위해 50ms 주기로 업데이트한다.
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */
    DisplayData_t localData = {0};
    uint32_t last_oled_update_tick = 0; // 마지막 OLED 업데이트 시간을 기록

  for(;;)
  {
	  // DisplayDataQueue에 데이터가 들어올 때까지 무한정 대기한다.
	  if (osMessageQueueGet(DisplayDataQueueHandle, &localData, NULL, osWaitForever) == osOK)
	  {
		  // LED는 즉각적인 반응이 중요하므로, 데이터 수신 즉시 상태를 업데이트한다.
		  LEDControl_Update(localData.status_ldr, localData.status_direction, localData.status_brake);

		  uint32_t current_tick = HAL_GetTick();

		  // OLED 화면은 50ms마다 한 번씩만 업데이트하여 I2C 버스 부하 및 화면 깜빡임을 줄인다.
		  if (current_tick - last_oled_update_tick >= 50)
		  {
			  last_oled_update_tick = current_tick; // 마지막 업데이트 시간 갱신

			  // 배터리 상태를 읽어온다.
			  float vout = 0.0f;
			  float percent = Read_Battery_Percentage(&vout);

			  // 전체 CAN 통신 상태를 종합한다 (Central과 Sensor 모두 정상이어야 함).
			  bool is_can_ok = localData.is_central_can_ok && localData.is_sensor_can_ok;

			  // 최종적으로 계산된 모든 정보를 OLED에 표시한다.
			  OLED_UpdateDisplay(percent, vout, is_can_ok, localData.rf_ok);
		  }
	  }
  }
  /* USER CODE END StartDisplayTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

