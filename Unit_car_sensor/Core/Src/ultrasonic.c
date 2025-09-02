/**
 * @file ultrasonic.c
 * @brief 초음파 센서 드라이버 소스 파일
 * @author YeonsuJ
 * @date 2025-07-26
 * @note TIM2를 us 단위 지연(delay)에 사용하고, TIM4를 입력 캡처(Input Capture)에 사용한다.
 */

#include "ultrasonic.h"
#include "tim.h"

/* --- 외부 핸들 --- */
extern TIM_HandleTypeDef htim2; // us 지연에 사용될 타이머 핸들
extern TIM_HandleTypeDef htim4; // 입력 캡처에 사용될 타이머 핸들

// --- 전역 변수 ---
volatile uint32_t ic_val1_front = 0;           // 전방 센서 ECHO 핀의 상승 엣지 시점 타이머 값
volatile uint32_t ic_val2_front = 0;           // 전방 센서 ECHO 핀의 하강 엣지 시점 타이머 값
volatile uint8_t is_first_captured_front = 0;  // 전방 센서 ECHO 핀의 상승 엣지 감지 여부 플래그
volatile uint32_t distance_front = 0;          // 계산된 전방 거리 (cm)

volatile uint32_t ic_val1_rear = 0;            // 후방 센서 ECHO 핀의 상승 엣지 시점 타이머 값
volatile uint32_t ic_val2_rear = 0;            // 후방 센서 ECHO 핀의 하강 엣지 시점 타이머 값
volatile uint8_t is_first_captured_rear = 0;   // 후방 센서 ECHO 핀의 상승 엣지 감지 여부 플래그
volatile uint32_t distance_rear = 0;           // 계산된 후방 거리 (cm)

/**
 * @brief 마이크로초(us) 단위의 지연을 생성한다.
 * @param us 지연시킬 시간 (마이크로초)
 * @note TIM2를 사용하며, 블로킹 방식으로 지연을 생성한다.
 */
void delay_us(uint16_t us)
{
  __HAL_TIM_SET_COUNTER(&htim2, 0);
  while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

/**
 * @brief 초음파 센서 사용에 필요한 타이머를 초기화하고 시작한다.
 * @note TIM2는 delay_us 함수를 위해, TIM4는 입력 캡처를 위해 사용된다.
 */
void Ultrasonic_Init(void)
{
	HAL_TIM_Base_Start(&htim2);  // delay_us를 위한 타이머 카운터 시작
	HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_1); // 전방 센서 입력 캡처 인터럽트 시작 (CH1)
	HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_2); // 후방 센서 입력 캡처 인터럽트 시작 (CH2)
}

/**
 * @brief 전방 및 후방 초음파 센서에 트리거 펄스를 전송하여 거리 측정을 시작한다.
 */
void Ultrasonic_Trigger(void)
{
	// 1) 전방 트리거 핀에 10us 펄스 출력
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
	delay_us(10);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);

	// 2) 후방 트리거 핀에 10us 펄스 출력
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	delay_us(10);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

/**
 * @brief 타이머 입력 캡처 인터럽트 콜백 함수이다.
 * @param htim 콜백을 발생시킨 타이머의 핸들
 * @note 전방(TIM4_CH1) 및 후방(TIM4_CH2) 초음파 센서의 ECHO 핀 신호를 감지하여 거리를 계산한다.
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
  {
    // 전방 센서(Channel 1)의 Echo 신호 처리
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
      if (is_first_captured_front == 0) // 상승 엣지 감지
      {
        ic_val1_front = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
        is_first_captured_front = 1;
      }
      else // 하강 엣지 감지
      {
        ic_val2_front = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
        
        // 거리 계산 중 인터럽트 방지를 위해 잠시 비활성화
        __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC1);
        uint32_t diff = (ic_val2_front > ic_val1_front) ? (ic_val2_front - ic_val1_front) : (0xFFFF - ic_val1_front + ic_val2_front);
        distance_front = (diff * 0.0343f) / 2.0f; // 음속 기반 거리 계산 (cm)
        is_first_captured_front = 0;  // 다음 측정을 위해 플래그 리셋
        __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC1); // 인터럽트 재활성화
      }
    }
    
    // 후방 센서(Channel 2)의 Echo 신호 처리
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
      if (is_first_captured_rear == 0) // 상승 엣지 감지
      {
        ic_val1_rear = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);
        is_first_captured_rear = 1;
      }
      else // 하강 엣지 감지
      {
        ic_val2_rear = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
        __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC2);
        uint32_t diff = (ic_val2_rear > ic_val1_rear) ? (ic_val2_rear - ic_val1_rear) : (0xFFFF - ic_val1_rear + ic_val2_rear);
        distance_rear = (diff * 0.0343f) / 2.0f;
        is_first_captured_rear = 0;
        __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC2);
      }
    }
  }
}
