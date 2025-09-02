// input_handler.h

#ifndef INC_INPUT_HANDLER_H_
#define INC_INPUT_HANDLER_H_

#include "main.h"

// 방향 전송값 정의 (main.c에서 이곳으로 이동)
typedef enum {
    LOCKER_FORWARD = 1,
    LOCKER_BACKWARD = 0
} LockerDirection;

/**
 * @brief  입력 처리 모듈을 초기화합니다.
 */
void InputHandler_Init(void);

/**
 * @brief  GPIO 인터럽트 발생 시 호출될 콜백 함수입니다.
 * @param  GPIO_Pin: 인터럽트를 발생시킨 핀 번호
 */
void InputHandler_GpioCallback(uint16_t GPIO_Pin);

/**
 * @brief  타이머 인터럽트 발생 시 호출될 콜백 함수입니다.
 * 이 함수는 20ms마다 호출되는 것을 가정합니다.
 */
void InputHandler_TimerCallback(void);

/**
 * @brief  엑셀 버튼이 눌린 누적 시간을 ms 단위로 반환합니다.
 * @retval uint16_t: 엑셀 눌림 시간 (ms)
 */
uint16_t InputHandler_GetAccelMillis(void);

/**
 * @brief  브레이크 버튼이 눌린 누적 시간을 ms 단위로 반환합니다.
 * @retval uint16_t: 브레이크 눌림 시간 (ms)
 */
uint16_t InputHandler_GetBrakeMillis(void);

/**
 * @brief  현재 방향 스위치의 상태를 반환합니다.
 * @retval LockerDirection: 현재 방향 (FORWARD or BACKWARD)
 */
LockerDirection InputHandler_GetDirection(void);


#endif /* INC_INPUT_HANDLER_H_ */
