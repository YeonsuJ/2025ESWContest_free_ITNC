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
#include "can_handler.h"
#include "ultrasonic.h"
#include "motor_encoder.h"
#include "tim.h"
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

/* USER CODE END Variables */
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for CANTask */
osThreadId_t CANTaskHandle;
const osThreadAttr_t CANTask_attributes = {
  .name = "CANTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CANTxQueue */
osMessageQueueId_t CANTxQueueHandle;
const osMessageQueueAttr_t CANTxQueue_attributes = {
  .name = "CANTxQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartSensorTask(void *argument);
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

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of CANTxQueue */
  CANTxQueueHandle = osMessageQueueNew (4, sizeof(SensorData_t), &CANTxQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* creation of CANTask */
  CANTaskHandle = osThreadNew(StartCANTask, NULL, &CANTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartSensorTask */
/**
  * @brief SensorTask는 주기적으로 각종 센서 데이터를 수집하여 CANTask로 전달하는 역할을 한다.
  * @note 10ms 주기로 동작하며, 초음파 센서, 엔코더(RPM), 조도 센서 값을 읽어와 큐(Queue)를 통해 전송한다.
  */
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
	// --- SensorTask 초기화 ---
	Ultrasonic_Init(); // 초음파 센서 관련 타이머(TIM2, TIM4)를 초기화하고 시작한다.
	HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL); // 엔코더 입력을 위한 타이머(TIM1)를 시작한다.

	// osDelayUntil을 사용하기 위한 변수
	uint32_t last_wake_time = osKernelGetTickCount();
	const uint32_t period_ms = 10; // 10ms 주기
  /* Infinite loop */
	for(;;)
	  {
	    // 정확한 주기를 유지하기 위해 osDelayUntil을 사용한다.
	    // 다음 깨어날 시간을 계산한 후, 그 시간까지 Task를 지연시킨다.
	    last_wake_time += period_ms;
	    osDelayUntil(last_wake_time);

	    // 센서 데이터 수집
	    Update_Motor_RPM();   // 엔코더 값을 읽어 RPM을 계산한다.
	    Ultrasonic_Trigger(); // 초음파 센서 거리 측정을 시작한다.

	    SensorData_t sensor_packet; // CANTask로 전송할 데이터 패킷 구조체

	    // 데이터 패킷 채우기
	    sensor_packet.light_condition = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3); // 조도 센서 값(GPIO)을 읽는다.
	    sensor_packet.rpm = MotorControl_GetRPM(); // 계산된 RPM 값을 가져온다.

	    // ISR(ultrasonic.c의 콜백 함수)에서 변경되는 전역 변수에 안전하게 접근하기 위해 Critical Section으로 보호한다.
	    taskENTER_CRITICAL();
	    sensor_packet.distance_front = distance_front; // 전방 거리 값을 복사한다.
	    sensor_packet.distance_rear = distance_rear;   // 후방 거리 값을 복사한다.
	    taskEXIT_CRITICAL();

	    // 채워진 데이터 패킷을 큐(CANTxQueueHandle)로 전송한다.
	    osMessageQueuePut(CANTxQueueHandle, &sensor_packet, 0, 0);
	  }
  /* USER CODE END StartSensorTask */
}

/* USER CODE BEGIN Header_StartCANTask */
/**
* @brief CANTask는 SensorTask로부터 큐를 통해 데이터를 수신하고, 이를 가공하여 CAN 버스로 전송하는 역할을 한다.
* @note 큐에 데이터가 들어올 때까지 대기(Block)하며, 수신된 데이터를 CAN 프로토콜에 맞는 형식으로 변환하여 송신한다.
*/
/* USER CODE END Header_StartCANTask */
void StartCANTask(void *argument)
{
  /* USER CODE BEGIN StartCANTask */

    CAN_tx_Init(); // CAN 통신 및 Tx 메시지 헤더를 초기화한다.

    SensorData_t received_packet; // SensorTask로부터 받을 데이터 패킷 구조체

    /* Infinite loop */
    for(;;)
    {
			// 큐(CANTxQueue)에 데이터가 들어올 때까지 무한정 대기한다.
			if (osMessageQueueGet(CANTxQueueHandle, &received_packet, NULL, osWaitForever) == osOK)
			{
        // --- 수신된 데이터를 CAN 전송 형식에 맞게 가공 ---

				// 1. 장애물 상태 데이터를 1바이트 비트마스크로 가공한다.
				uint8_t obstacle_status = 0; // 0으로 초기화 (장애물 없음)

				if (received_packet.distance_front <= 10) // 전방 10cm 이내에 장애물 감지 시
				{
					obstacle_status |= (1 << 0); // 0번 비트를 1로 세팅한다. (값: 1)
				}
				if (received_packet.distance_rear <= 10) // 후방 10cm 이내에 장애물 감지 시
				{
					obstacle_status |= (1 << 1); // 1번 비트를 1로 세팅한다. (값: 2)
				}
				TxData[0] = obstacle_status; // 최종 계산된 값을 TxData[0]에 할당한다.

				// 2. 조도 센서 상태를 1바이트로 가공한다 (SET이면 1, RESET이면 0)
				TxData[1] = (received_packet.light_condition == GPIO_PIN_SET);

				// 3. float 타입의 RPM 값을 2바이트 정수형으로 변환하여 저장한다.
				uint16_t rpm_value = (uint16_t)received_packet.rpm;
				// 하위 8비트(LSB)와 상위 8비트(MSB)로 분리한다.
				TxData[2] = (uint8_t)(rpm_value & 0x00FF);      // LSB
				TxData[3] = (uint8_t)((rpm_value >> 8) & 0x00FF); // MSB

				// 가공된 데이터가 담긴 TxData 배열을 CAN 버스로 전송한다.
				CAN_Send();
      }
    }
  /* USER CODE END StartCANTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

