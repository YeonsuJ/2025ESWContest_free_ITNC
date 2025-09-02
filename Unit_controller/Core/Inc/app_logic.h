#ifndef INC_APP_LOGIC_H_
#define INC_APP_LOGIC_H_

#include "main.h"
#include "cmsis_os.h"

// --- 공유 데이터 타입 정의 ---
typedef struct {
     uint16_t rpm;
     uint8_t direction;
     uint8_t comm_ok;
} DisplayData_t;


// --- 상수 정의 ---
// 태스크 실행 주기 (ms 단위)
#define SENSOR_TASK_PERIOD_MS 5
#define DISPLAY_TASK_PERIOD_MS 100

// 디스플레이 속성
#define MAX_RPM 300.0f

// 패킷 구조 인덱스
#define PACKET_IDX_ID         0
#define PACKET_IDX_ROLL       1 // int16_t, 2 bytes
#define PACKET_IDX_ACCEL_MS   3 // uint16_t, 2 bytes
#define PACKET_IDX_BRAKE_MS   5 // uint16_t, 2 bytes
#define PACKET_IDX_DIRECTION  7 // uint8_t, 1 byte


// --- 공유 변수 (app_logic.c 또는 freertos.c에 정의됨) ---
extern DisplayData_t g_displayData;
extern osMutexId_t g_displayDataMutexHandle;


// --- 함수 프로토타입 ---
float App_GetRollAngle(void);
void App_BuildPacket(uint8_t* packet_buffer, float roll_angle);
void App_HandleAckPayload(uint8_t* ack_payload);


#endif /* INC_APP_LOGIC_H_ */
