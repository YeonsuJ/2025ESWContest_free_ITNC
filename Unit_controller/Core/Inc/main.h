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
#define CSN_Pin GPIO_PIN_3
#define CSN_GPIO_Port GPIOA
#define CE_Pin GPIO_PIN_4
#define CE_GPIO_Port GPIOA
#define BUTTON_accel_Pin GPIO_PIN_0
#define BUTTON_accel_GPIO_Port GPIOB
#define BUTTON_accel_EXTI_IRQn EXTI0_IRQn
#define BUTTON_brake_Pin GPIO_PIN_1
#define BUTTON_brake_GPIO_Port GPIOB
#define BUTTON_brake_EXTI_IRQn EXTI1_IRQn
#define Gyro_SCL_Pin GPIO_PIN_10
#define Gyro_SCL_GPIO_Port GPIOB
#define Gyro_SDA_Pin GPIO_PIN_11
#define Gyro_SDA_GPIO_Port GPIOB
#define HAPTIC_Pin GPIO_PIN_8
#define HAPTIC_GPIO_Port GPIOA
#define IRQ_Pin GPIO_PIN_3
#define IRQ_GPIO_Port GPIOB
#define IRQ_EXTI_IRQn EXTI3_IRQn
#define OLED_SCK_Pin GPIO_PIN_6
#define OLED_SCK_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_7
#define OLED_SDA_GPIO_Port GPIOB
#define Motor_Backward_Pin GPIO_PIN_8
#define Motor_Backward_GPIO_Port GPIOB
#define Motor_Foward_Pin GPIO_PIN_9
#define Motor_Foward_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
