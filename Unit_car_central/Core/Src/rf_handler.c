/**
 * @file rf_handler.c
 * @brief NRF24L01+ RF 모듈 제어 로직을 구현한다.
 * @author YeonsuJ
 * @date 2025-07-22
 * @note FreeRTOS 세마포어를 사용하여 인터럽트 기반의 비동기 데이터 수신을 처리한다.
 */
#include "rf_handler.h"
#include "NRF24.h" 
#include "NRF24_reg_addresses.h"
#include <string.h>
#include "cmsis_os.h"

/**
 * @brief NRF24 수신(Rx) 패킷 구조 정의
 * @details
 * Byte | 내용        | 타입      | 비고            |
 * 0    | 메세지ID    | (uint8_t) | 1: 주행명령     |
 * 1~2  | roll       | int16_t    | x100 인코딩    |
 * 3~4  | accel_ms   | uint16_t   |                |
 * 5~6  | brake_ms   | uint16_t   |                |
 * 7    | direction  | uint8_t    | 0:후진, 1:전진  |
 * 8~31 | 예약/미사용 |            |                |
 */

// 페이로드 크기 정의
#define RX_PAYLOAD_SIZE 8   // 수신할 데이터의 크기 (Byte)
#define ACK_PAYLOAD_SIZE 3  // 송신할 ACK 페이로드의 크기 (Byte)

/**
 * @brief RF 데이터 수신 인터럽트 처리를 위한 FreeRTOS 세마포어 핸들
 * @note 이 세마포어는 `freertos.c`에서 생성되고, IRQ 콜백에서 release된다.
 */
extern osSemaphoreId_t RFSemHandle;

/**
 * @brief 조종기로 보낼 ACK 응답 데이터를 저장하는 내부 버퍼
 */
static uint8_t ack_response[ACK_PAYLOAD_SIZE] = {0};

/**
 * @brief NRF24 모듈을 수신(Rx) 모드로 초기화한다.
 * @note 주소, 채널, 데이터 속도 등 통신 파라미터를 설정하고,
 * ACK 페이로드 기능을 활성화한 후 수신 대기 모드로 전환한다.
 */
void RFHandler_Init(void)
{
    static uint8_t rx_addr[5] = {0x45, 0x55, 0x67, 0x10, 0x21};

    csn_high();
    HAL_Delay(5);
    ce_low();

    nrf24_init();
    nrf24_auto_ack_all(auto_ack);    // 모든 파이프에 대해 자동 ACK 활성화
    nrf24_en_ack_pld(enable);        // ACK 페이로드 기능 활성화
    nrf24_dpl(disable);              // 동적 페이로드 길이 비활성화
    nrf24_set_crc(enable, _1byte);   // 1바이트 CRC 활성화
    nrf24_tx_pwr(_0dbm);             // 송신 출력 0dBm 설정
    nrf24_data_rate(_2mbps);         // 데이터 속도 2Mbps 설정 (조종기와 동일하게)
    nrf24_set_channel(90);           // RF 채널 90번 설정
    nrf24_set_addr_width(5);         // 주소 폭 5바이트 설정
    nrf24_open_rx_pipe(1, rx_addr);  // 수신 파이프 1번 열기
    nrf24_pipe_pld_size(1, RX_PAYLOAD_SIZE); // 파이프 1번의 페이로드 크기 설정

    nrf24_listen(); // 수신 대기 시작
}

/**
 * @brief NRF24 모듈의 IRQ 핀 외부 인터럽트(EXTI) 발생 시 호출되는 콜백 함수
 * @note 이 함수는 ISR 컨텍스트에서 실행된다.
 * 데이터 수신이 완료되었음을 태스크에 알리기 위해 `RFSemHandle` 세마포어를 반환(release)한다.
 * 커널이 실행 중이고 핸들이 유효할 때만 ISR 안전 함수인 `osSemaphoreRelease`를 호출한다.
 */
void RFHandler_IrqCallback(void)
{
    if (osKernelGetState() == osKernelRunning && RFSemHandle) {
        (void)osSemaphoreRelease(RFSemHandle);
    }
}

/**
 * @brief 다음에 전송할 ACK 페이로드에 포함될 데이터를 설정한다.
 * @param payload 전송할 데이터가 담긴 버퍼의 포인터
 * @param length 전송할 데이터의 길이
 * @note 버퍼 오버플로우를 방지하기 위해, 복사할 길이는 내부 ACK 버퍼 크기(`ACK_PAYLOAD_SIZE`)를 초과할 수 없다.
 */
void RFHandler_SetAckPayload(uint8_t* payload, uint8_t length)
{
	// 1. 복사할 길이를 결정 (버퍼 오버플로우 방지)
	uint8_t len_to_copy = length;
	if (len_to_copy > ACK_PAYLOAD_SIZE)
		len_to_copy = ACK_PAYLOAD_SIZE; // ACK 버퍼 크기보다 크면 잘라냄

	// 2. 전달받은 payload 데이터를 내부 ack_response 버퍼로 복사
	memcpy(ack_response, payload, len_to_copy);
}

/**
 * @brief 새로운 주행 명령이 수신되었는지 확인하고, 있다면 데이터를 파싱하여 반환한다.
 * @param command 파싱된 명령 데이터를 저장할 `VehicleCommand_t` 구조체의 포인터
 * @retval true 새로운 명령이 수신되어 파싱까지 성공했음을 의미한다.
 * @retval false 수신된 데이터가 없거나, 수신된 데이터의 ID가 유효하지 않음을 의미한다.
 * @note NRF24 상태 레지스터의 RX_DR (수신 완료) 비트를 확인하여 새 데이터 유무를 판단한다.
 * 데이터를 성공적으로 읽은 후에는 ACK 페이로드를 전송하고 RX_DR 플래그를 클리어한다.
 */
bool RFHandler_GetNewCommand(VehicleCommand_t* command)
{
    // 수신 데이터 준비 여부 확인
    uint8_t status = nrf24_r_status();
    if ((status & (1U << RX_DR)) == 0U) {
        return false; // 새 데이터 없음
    }

    // 데이터 수신
    uint8_t rx_buffer[RX_PAYLOAD_SIZE] = {0};
    nrf24_receive(rx_buffer, RX_PAYLOAD_SIZE);

    // 미리 준비된 ACK 페이로드 송신
    nrf24_transmit_rx_ack_pld(1, ack_response, ACK_PAYLOAD_SIZE);

    // RX_DR 클리어
    nrf24_clear_rx_dr();

    // 파싱
    if (rx_buffer[0] == 1)  // ID == 1 : 주행 명령
    {
        int16_t roll_encoded = 0;
        memcpy(&roll_encoded, &rx_buffer[1], sizeof(int16_t));
        command->roll = ((float)roll_encoded) / 100.0f;

        memcpy(&command->accel_ms,  &rx_buffer[3], sizeof(uint16_t));
        memcpy(&command->brake_ms,  &rx_buffer[5], sizeof(uint16_t));
        command->direction = rx_buffer[7];

        return true;
    }

    return false; // 유효하지 않은 ID
}
