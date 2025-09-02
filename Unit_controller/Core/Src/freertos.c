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


/* USER CODE BEGIN Header_StartsensorTask */
/**
* @brief 주기적으로 롤(Roll) 각도 센서 값을 측정하여 다른 태스크로 전달하는 태스크
* @param argument: None
* @retval None
* @note 이 태스크는 다음과 같은 순서로 동작한다:
* 1. `App_GetRollAngle` 함수를 호출하여 현재 차량의 롤 각도를 얻음.
* 2. `osMessageQueuePut`을 사용하여 측정된 롤 각도 값을 `sensorQueueHandle` 메시지 큐에 전송한다.
* 이를 통해 commTask가 이 값을 사용할 수 있다.
* 3. `osDelay`를 사용하여 `SENSOR_TASK_PERIOD_MS` (5ms) 만큼 대기하며, 일정한 주기로 센서 값을 측정하도록 보장한다.
*/
/* USER CODE END Header_StartsensorTask */
void StartsensorTask(void *argument)
{
  /* USER CODE BEGIN StartsensorTask */
  /* Infinite loop */
  for(;;)
  {
    float roll = App_GetRollAngle(); // IMU 센서로부터 roll값 갱신

    osMessageQueuePut(sensorQueueHandle, &roll, 0U, 0U); // 큐를 통해 송신 태스크(commTask)로 전송

    osDelay(SENSOR_TASK_PERIOD_MS); // 5ms 주기 대기
  }
  /* USER CODE END StartsensorTask */
}

/* USER CODE BEGIN Header_StartcommTask */
/**
  * @brief  센서 태스크로부터 롤(Roll) 각도 값을 받아 통신 패킷을 생성하고 전송하는 태스크다.
  * @param  argument: None
  * @retval None
  * @note   이 태스크는 다음과 같은 순서로 동작한다:
  * 1. `sensorQueueHandle` 메시지 큐에 새로운 데이터가 들어올 때까지 무한 대기한다.
  * 2. 큐에서 롤 각도 값을 성공적으로 수신하면, 이 값을 이용해 전송용 패킷을 만든다.
  * 3. 완성된 패킷을 통신 핸들러를 통해 외부로 전송한다.
  * 4. 위 과정을 무한 반복한다.
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
     osMessageQueueGet(sensorQueueHandle, &roll, NULL, osWaitForever); // 블로킹 상태로 대기 및 roll 값 수신

     App_BuildPacket(tx_packet, roll); // 데이터 패키징

     CommHandler_Transmit(tx_packet, PAYLOAD_SIZE); // 차량부로 패킷 전송
  }
  /* USER CODE END StartcommTask */
}

/* USER CODE BEGIN Header_StartackHandlerTask */
/**
* @brief  차량의 통신 모듈로부터 ACK(응답) 수신 인터럽트를 처리하는 태스크다.
* @param  argument: 사용되지 않는다.
* @retval None
* @note   이 태스크는 통신 인터럽트가 발생할 때마다 동작하며, 다음과 같은 순서로 실행된다:
* 1. `ackSemHandle` 세마포어를 통해 통신 모듈의 전송 완료(TX DR) 또는 최대 재전송 실패(MAX_RT) 인터럽트가 발생하기를 기다린다.
* 2. 인터럽트가 발생하면 `CommHandler_CheckStatus`를 호출하여 통신 상태(성공/실패)를 확인하고, 수신된 ACK 페이로드를 `ack_packet` 버퍼에 저장한다.
* 3. 통신이 성공했다면(COMM_TX_SUCCESS), `App_HandleAckPayload`를 호출하여 ACK 페이로드 데이터를 처리하고, 뮤텍스를 사용하여 공유 변수 `g_displayData.comm_ok`를 1(성공)로 업데이트한다.
* 4. 통신이 실패했다면(COMM_TX_FAIL), 뮤텍스를 사용하여 `g_displayData.comm_ok`를 0(실패)으로 업데이트한다.
*/
/* USER CODE END Header_StartackHandlerTask */
void StartackHandlerTask(void *argument)
{
  /* USER CODE BEGIN StartackHandlerTask */
  uint8_t ack_packet[ACK_PAYLOAD_SIZE] = {0}; // ACK 응답 신호용 버퍼

  /* Infinite loop */
  for(;;)
  {
      // ACK 관련 인터럽트(TX DR 또는 MAX_RT)가 발생할 때까지 세마포어를 기다린다.
      osSemaphoreAcquire(ackSemHandle, osWaitForever); // ACK 응답 신호 인터럽트 발생 대기

      CommStatus_t status = CommHandler_CheckStatus(ack_packet, ACK_PAYLOAD_SIZE); // 통신 상태 확인 및 응답 패킷 복제


      if (status == COMM_TX_SUCCESS)
      {
          App_HandleAckPayload(ack_packet); // 진동 동작 판단

          if (osMutexAcquire(g_displayDataMutexHandle, 10) == osOK) // 뮤택스를 통해 공유변수 접근
          {
              g_displayData.comm_ok = 1; // 1: 통신 정상
              osMutexRelease(g_displayDataMutexHandle);
          }
      }

      else if (status == COMM_TX_FAIL)
      {
          if (osMutexAcquire(g_displayDataMutexHandle, 10) == osOK)
          {
              g_displayData.comm_ok = 0; // 0: 통신 실패
              osMutexRelease(g_displayDataMutexHandle);
          }
      }
  }
  /* USER CODE END StartackHandlerTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief  공유 데이터를 기반으로 OLED 디스플레이에 정보를 주기적으로 표시하는 태스크다.
* @param  argument: None
* @retval None
* @note   이 태스크는 다음과 같은 순서로 동작한다:
* 1. 뮤텍스를 사용하여 다른 태스크와 공유하는 `g_displayData`의 데이터를 안전하게 로컬 변수로 복사한다.
* 2. 통신 상태(`comm_ok`)를 확인한다.
* 3. 통신이 정상이면, RPM 값을 백분율로 변환하고 주행 방향(D/R)을 문자열로 만들어 화면에 표시한다.
* 4. 통신이 두절된 상태이면, 화면에 "NO SIGNAL" 메시지를 표시한다.
* 5. `DISPLAY_TASK_PERIOD_MS` (100ms) 주기로 위 과정을 반복하여 디스플레이를 갱신한다.
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */
   DisplayData_t localDisplayData = {0};
   char speed_str[16];
   char dir_str[10];
   int speed_percentage;

   if (osMutexAcquire(g_displayDataMutexHandle, osWaitForever) == osOK)
   {
       g_displayData.comm_ok = 1; // 초기 설정값 : 통신 성공
       osMutexRelease(g_displayDataMutexHandle);
   }

   /* Infinite loop */
   for(;;)
   {
     if (osMutexAcquire(g_displayDataMutexHandle, osWaitForever) == osOK) // 뮤택스를 통해 공유변수 접근
     {
         localDisplayData = g_displayData;
         osMutexRelease(g_displayDataMutexHandle);
     }

     SSD1306_Fill(SSD1306_COLOR_BLACK);

     if (localDisplayData.comm_ok) // 통신 정상
     {

         speed_percentage = (int)(((float)localDisplayData.rpm / MAX_RPM) * 100.0f);  // RPM -> 백분율로 변환
         if (speed_percentage > 100) { speed_percentage = 100; }

         sprintf(speed_str, "%d %%", speed_percentage); // 속도 출력


         switch(localDisplayData.direction)
         {
             case 0: strcpy(dir_str, "   R"); break; // 후진
             case 1: strcpy(dir_str, "   D"); break; // 전진
             default: strcpy(dir_str, ""); break;
         }

         if (localDisplayData.direction == 1) { SSD1306_GotoXY(26, 0); }
         else if (localDisplayData.direction == 0) { SSD1306_GotoXY(20, 0); }
         SSD1306_Puts(dir_str, &Font_11x18, 1); // 기어 상태 출력 (D or R)

         SSD1306_GotoXY(39, 20);
         SSD1306_Puts("SPEED", &Font_11x18, 1);


         uint8_t len = strlen(speed_str);
         uint8_t x_pos = (128 - (len * 7)) / 2;
         SSD1306_GotoXY(x_pos, 40);
         SSD1306_Puts(speed_str, &Font_11x18, 1);
     }
     else
     {
         SSD1306_GotoXY(5, 25);
         SSD1306_Puts("NO SIGNAL", &Font_11x18, 1); // 통신 실패
     }
     SSD1306_UpdateScreen();
     osDelay(DISPLAY_TASK_PERIOD_MS); // 100ms 주기
   }
  /* USER CODE END StartDisplayTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
