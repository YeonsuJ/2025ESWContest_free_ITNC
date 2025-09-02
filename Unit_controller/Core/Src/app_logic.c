#include "main.h"
#include "cmsis_os.h"
#include <string.h>
#include "app_logic.h"
#include "comm_handler.h"
#include "input_handler.h"
#include "mpu6050.h"

// Private variables from freertos.c that are needed here
extern I2C_HandleTypeDef hi2c2;
extern osMutexId_t g_displayDataMutexHandle;

// The MPU6050 instance is now local to this file
static MPU6050_t MPU6050;

// The display data struct is also managed here now
DisplayData_t g_displayData = {0};


float App_GetRollAngle(void) // roll 데이터 수집 함수
{
    MPU6050_Read_All(&hi2c2, &MPU6050);
    return MPU6050.KalmanAngleX;
}

void App_BuildPacket(uint8_t* packet_buffer, float roll_angle) // 데이터 패키징 함수
{
    memset(packet_buffer, 0, PAYLOAD_SIZE);

    packet_buffer[PACKET_IDX_ID] = 1; // [0]

    int16_t roll_encoded = (int16_t)(roll_angle * 100.0f);
    memcpy(&packet_buffer[PACKET_IDX_ROLL], &roll_encoded, sizeof(int16_t)); // [1], [2]


    uint16_t accel_ms = InputHandler_GetAccelMillis();
    uint16_t brake_ms = InputHandler_GetBrakeMillis();
    memcpy(&packet_buffer[PACKET_IDX_ACCEL_MS], &accel_ms, sizeof(uint16_t)); // [3], [4]
    memcpy(&packet_buffer[PACKET_IDX_BRAKE_MS], &brake_ms, sizeof(uint16_t)); // [5], [6]

    uint8_t current_direction = InputHandler_GetDirection();
    packet_buffer[PACKET_IDX_DIRECTION] = current_direction; // [7]

    if (g_displayDataMutexHandle != NULL)
    {
    	if (osMutexAcquire(g_displayDataMutexHandle, 10) == osOK) // displayTask와 충돌 방지 뮤텍스 보호
    	{
    		g_displayData.direction = current_direction;
            osMutexRelease(g_displayDataMutexHandle);
        }
    }
}

 void App_HandleAckPayload(uint8_t* ack_payload) // ACK 신호 기반 진동 생성 함수
 {
     if (ack_payload[0] == 1)
     {
         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
     }
     else
     {
         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
     }

     if (g_displayDataMutexHandle != NULL)
     {
         if (osMutexAcquire(g_displayDataMutexHandle, 10) == osOK)
         {
             memcpy(&g_displayData.rpm, &ack_payload[1], sizeof(uint16_t));
             osMutexRelease(g_displayDataMutexHandle);
         }
     }
 }
