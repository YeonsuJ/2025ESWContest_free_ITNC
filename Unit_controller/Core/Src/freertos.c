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
#include <string.h>
#include <stdio.h>
#include "comm_handler.h"
#include "input_handler.h"
#include "ssd1306.h"
#include "fonts.h"
#include "app_logic.h" // Use the new application logic header

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

// Mutex for display data is defined here as it's an RTOS object
   osMutexId_t g_displayDataMutexHandle;
   const osMutexAttr_t g_displayDataMutex_attributes = {
     .name = "displayDataMutex"
   };
/* USER CODE END Variables */
/* Definitions for commTask */
osThreadId_t commTaskHandle;
const osThreadAttr_t commTask_attributes = {
  .name = "commTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for sensorTask */
osThreadId_t sensorTaskHandle;
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for ackHandlerTask */
osThreadId_t ackHandlerTaskHandle;
const osThreadAttr_t ackHandlerTask_attributes = {
  .name = "ackHandlerTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensorQueue */
osMessageQueueId_t sensorQueueHandle;
const osMessageQueueAttr_t sensorQueue_attributes = {
  .name = "sensorQueue"
};
/* Definitions for ackSem */
osSemaphoreId_t ackSemHandle;
const osSemaphoreAttr_t ackSem_attributes = {
  .name = "ackSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartcommTask(void *argument);
void StartsensorTask(void *argument);
void StartackHandlerTask(void *argument);
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
  g_displayDataMutexHandle = osMutexNew(&g_displayDataMutex_attributes);
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of ackSem */
  ackSemHandle = osSemaphoreNew(1, 0, &ackSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of sensorQueue */
  sensorQueueHandle = osMessageQueueNew (1, sizeof(float), &sensorQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of commTask */
  commTaskHandle = osThreadNew(StartcommTask, NULL, &commTask_attributes);

  /* creation of sensorTask */
  sensorTaskHandle = osThreadNew(StartsensorTask, NULL, &sensorTask_attributes);

  /* creation of ackHandlerTask */
  ackHandlerTaskHandle = osThreadNew(StartackHandlerTask, NULL, &ackHandlerTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(StartDisplayTask, NULL, &DisplayTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartcommTask */
/**
  * @brief  Function implementing the commTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartcommTask */
void StartcommTask(void *argument)
{
  /* USER CODE BEGIN StartcommTask */
	  uint8_t tx_packet[PAYLOAD_SIZE] = {0};
	  float roll = 0.0f;

  /* Infinite loop */
  for(;;)
  {
	   osMessageQueueGet(sensorQueueHandle, &roll, NULL, osWaitForever);
	   App_BuildPacket(tx_packet, roll);
	   CommHandler_Transmit(tx_packet, PAYLOAD_SIZE);
  }
  /* USER CODE END StartcommTask */
}

/* USER CODE BEGIN Header_StartsensorTask */
/**
* @brief Function implementing the sensorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartsensorTask */
void StartsensorTask(void *argument)
{
  /* USER CODE BEGIN StartsensorTask */
  /* Infinite loop */
  for(;;)
  {
	    // 1. 센서 값을 읽어옵니다.
	    float roll = App_GetRollAngle();
	    // 2. 읽어온 값을 메시지 큐에 넣습니다.
	    osMessageQueuePut(sensorQueueHandle, &roll, 0U, 0U);

	    // 3. 다음 센서 측정까지 대기
	    osDelay(SENSOR_TASK_PERIOD_MS);
  }
  /* USER CODE END StartsensorTask */
}

/* USER CODE BEGIN Header_StartackHandlerTask */
/**
* @brief Function implementing the ackHandlerTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartackHandlerTask */
void StartackHandlerTask(void *argument)
{
  /* USER CODE BEGIN StartackHandlerTask */
	  uint8_t ack_packet[ACK_PAYLOAD_SIZE] = {0};

	  /* Infinite loop */
	  for(;;)
	  {
		  osSemaphoreAcquire(ackSemHandle, osWaitForever);
		  CommStatus_t status = CommHandler_CheckStatus(ack_packet, ACK_PAYLOAD_SIZE);

		  if (status == COMM_TX_SUCCESS)
		  {
			  // Process the received data
			  App_HandleAckPayload(ack_packet);

			  // Update comm status to OK
			  if (osMutexAcquire(g_displayDataMutexHandle, 10) == osOK)
			  {
				  g_displayData.comm_ok = 1;
				  osMutexRelease(g_displayDataMutexHandle);
			  }
		  }
		  else if (status == COMM_TX_FAIL)
		  {
			  // Update comm status to FAIL
			  if (osMutexAcquire(g_displayDataMutexHandle, 10) == osOK)
			  {
				  g_displayData.comm_ok = 0;
				  osMutexRelease(g_displayDataMutexHandle);
			  }
		  }
	  }
  /* USER CODE END StartackHandlerTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */
   DisplayData_t localDisplayData = {0};
   char speed_str[16];
   char dir_str[10];
   int speed_percentage;

   // Set initial communication status to OK
   if (osMutexAcquire(g_displayDataMutexHandle, osWaitForever) == osOK)
   {
	   g_displayData.comm_ok = 1;
	   osMutexRelease(g_displayDataMutexHandle);
   }

   /* Infinite loop */
   for(;;)
   {
	 // 1. Copy shared data under mutex protection
	 if (osMutexAcquire(g_displayDataMutexHandle, osWaitForever) == osOK)
	 {
		 localDisplayData = g_displayData;
		 osMutexRelease(g_displayDataMutexHandle);
	 }

	 SSD1306_Fill(SSD1306_COLOR_BLACK);

	 // 2. Check comm status and render display accordingly
	 if (localDisplayData.comm_ok)
	 {
		 // --- Normal Display ---
		 speed_percentage = (int)(((float)localDisplayData.rpm / MAX_RPM) * 100.0f);
		 if (speed_percentage > 100) { speed_percentage = 100; }
		 sprintf(speed_str, "%d %%", speed_percentage);

		 switch(localDisplayData.direction)
		 {
			 case 0: strcpy(dir_str, "   R"); break;
			 case 1: strcpy(dir_str, "   D"); break;
			 default: strcpy(dir_str, ""); break;
		 }

		 if (localDisplayData.direction == 1) { SSD1306_GotoXY(26, 0); }
		 else if (localDisplayData.direction == 0) { SSD1306_GotoXY(20, 0); }
		 SSD1306_Puts(dir_str, &Font_11x18, 1);

		 SSD1306_GotoXY(39, 20);
		 SSD1306_Puts("SPEED", &Font_11x18, 1);

		 uint8_t len = strlen(speed_str);
		 uint8_t x_pos = (128 - (len * 7)) / 2;
		 SSD1306_GotoXY(x_pos, 40);
		 SSD1306_Puts(speed_str, &Font_11x18, 1);
	 }
	 else
	 {
		 // --- Communication Failure Display ---
		 SSD1306_GotoXY(5, 25);
		 SSD1306_Puts("NO SIGNAL", &Font_11x18, 1);
	 }

	 SSD1306_UpdateScreen();

	 osDelay(DISPLAY_TASK_PERIOD_MS);
   }
  /* USER CODE END StartDisplayTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
