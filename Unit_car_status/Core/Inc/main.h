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

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
// CAN 통신 상태를 나타내기 위한 열거형
typedef enum {
    CAN_STATUS_OK,
    CAN_STATUS_HW_ERROR,
    CAN_STATUS_TIMEOUT
} CanStatus_t;

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
#define Brake_Left_LED_Pin GPIO_PIN_15
#define Brake_Left_LED_GPIO_Port GPIOB
#define Brake_Right_LED_Pin GPIO_PIN_8
#define Brake_Right_LED_GPIO_Port GPIOA
#define Front_Left_LED_Pin GPIO_PIN_9
#define Front_Left_LED_GPIO_Port GPIOA
#define Front_Right_LED_Pin GPIO_PIN_10
#define Front_Right_LED_GPIO_Port GPIOA
#define Rear_Left_LED_Pin GPIO_PIN_11
#define Rear_Left_LED_GPIO_Port GPIOA
#define Rear_Right_LED_Pin GPIO_PIN_12
#define Rear_Right_LED_GPIO_Port GPIOA
#define OLED_SCL_Pin GPIO_PIN_6
#define OLED_SCL_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_7
#define OLED_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
