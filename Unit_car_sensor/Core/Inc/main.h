/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// 센서 데이터 패킷 구조체 정의
#include "can_handler.h"
#include "ultrasonic.h"
#include "motor_encoder.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Rear_Trig_Pin GPIO_PIN_12
#define Rear_Trig_GPIO_Port GPIOB
#define Front_Trig_Pin GPIO_PIN_13
#define Front_Trig_GPIO_Port GPIOB
#define Encoder_A_Pin GPIO_PIN_8
#define Encoder_A_GPIO_Port GPIOA
#define Encoder_B_Pin GPIO_PIN_9
#define Encoder_B_GPIO_Port GPIOA
#define LDR_IN_Pin GPIO_PIN_3
#define LDR_IN_GPIO_Port GPIOB
#define Front_Echo_Pin GPIO_PIN_6
#define Front_Echo_GPIO_Port GPIOB
#define Rear_Echo_Pin GPIO_PIN_7
#define Rear_Echo_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
