/**
 * @file comm_handler.c
 * @brief NRF24L01+ RF 모듈을 송신(Tx) 모드로 제어하는 로직을 구현한다.
 * @author GeonKim
 * @date 2025-09-02
 * @note 이 핸들러는 데이터 패킷을 송신하고, 수신측으로부터 ACK 페이로드를 받는 역할을 담당한다.
 * 인터럽트 처리는 세마포어 대신 플래그(flag) 방식을 사용한다.
 */

#include "comm_handler.h"
#include "NRF24.h"
#include "NRF24_reg_addresses.h"

/**
 * @brief NRF24 송신(Tx) 패킷 구조 정의
 * @details
 * Byte | 내용        | 타입       | 비고            |
 * 0    | 메세지ID    | (uint8_t)  | 1: 주행명령     |
 * 1~2  | roll        | int16_t    | x100 인코딩     |
 * 3~4  | accel_ms    | uint16_t   |                 |
 * 5~6  | brake_ms    | uint16_t   |                 |
 * 7    | direction   | uint8_t    | 0:후진, 1:전진  |
 */

/**
 * @brief NRF24 수신(Rx) ACK 페이로드 구조 정의
 * @details
 * Byte | 내용        | 타입       | 비고                          |
 * 0    | haptic      | uint8_t    | 햅틱 피드백 신호              |
 * 1~2  | rpm         | uint16_t   | 모터 RPM (Little Endian)      |
 */


#define TX_PAYLOAD_SIZE  8  // 송신할 데이터의 크기 (Byte)
#define ACK_PAYLOAD_SIZE 3  // 수신할 ACK 페이로드의 크기 (Byte)

/**
 * @brief 수신측(차량)의 주소. 송신 파이프에 이 주소를 설정해야 한다.
 */
static uint8_t tx_addr[5] = {0x45, 0x55, 0x67, 0x10, 0x21};

/**
 * @brief NRF24 IRQ 발생을 알리는 플래그 변수
 * @note volatile 키워드를 사용하여 컴파일러 최적화로 인한 문제를 방지한다.
 * IRQ 콜백에서 1로 설정되고, CheckStatus 함수에서 0으로 클리어된다.
 */
static volatile uint8_t nrf_irq_flag = 0;

/**
 * @brief NRF24 모듈을 송신(Tx) 모드로 초기화한다.
 * @note 주소, 채널, 데이터 속도, 자동 재전송 등 통신 파라미터를 설정한다.
 * ACK 페이로드를 수신하기 위해 Rx 파이프 0번도 함께 설정한다.
 */
void CommHandler_Init(void)
{
    csn_high();
    HAL_Delay(5);
    ce_low();

    nrf24_init();
    nrf24_stop_listen();                // 송신 모드로 설정
    nrf24_auto_ack_all(auto_ack);       // 모든 파이프에 대해 자동 ACK 활성화
    nrf24_en_ack_pld(enable);           // ACK 페이로드 기능 활성화
    nrf24_dpl(disable);                 // 동적 페이로드 길이 비활성화
    nrf24_set_crc(enable, _1byte);      // 1바이트 CRC 활성화
    nrf24_tx_pwr(_0dbm);                // 송신 출력 0dBm 설정
    nrf24_data_rate(_2mbps);            // 데이터 속도 2Mbps 설정 (수신기와 동일하게)
    nrf24_set_channel(90);              // RF 채널 90번 설정
    nrf24_set_addr_width(5);            // 주소 폭 5바이트 설정
    nrf24_auto_retr_delay(4);           // 자동 재전송 딜레이 1000us (250 * (4+1))
    nrf24_auto_retr_limit(10);          // 자동 재전송 횟수 10회로 제한
    nrf24_open_tx_pipe(tx_addr);        // 송신 파이프 열기
    nrf24_open_rx_pipe(0, tx_addr);     // ACK 페이로드 수신을 위한 Rx 파이프 0번 열기
    nrf24_pipe_pld_size(0, ACK_PAYLOAD_SIZE); // 파이프 0번의 페이로드 크기 설정
}

/**
 * @brief NRF24 모듈의 IRQ 핀 외부 인터럽트(EXTI) 발생 시 호출되는 콜백 함수
 * @note 이 함수는 ISR 컨텍스트에서 실행된다.
 * 송신 완료 또는 실패 이벤트를 메인 로직에 알리기 위해 `nrf_irq_flag`를 1로 설정한다.
 */
void CommHandler_IrqCallback(void)
{
    nrf_irq_flag = 1;
}

/**
 * @brief 지정된 페이로드를 비동기 방식으로 송신한다.
 * @param payload 전송할 데이터가 담긴 버퍼의 포인터
 * @param len 전송할 데이터의 길이
 * @note 이 함수는 전송을 시작만 할 뿐, 완료를 기다리지 않는다.
 * 전송 결과는 IRQ 발생 후 `CommHandler_CheckStatus`를 통해 확인해야 한다.
 */
void CommHandler_Transmit(uint8_t* payload, uint8_t len)
{
    nrf24_transmit(payload, len);
}

/**
 * @brief IRQ 발생 후 통신 상태를 확인하고 결과를 반환한다.
 * @param ack_payload 수신된 ACK 페이로드를 저장할 버퍼의 포인터
 * @param len `ack_payload` 버퍼의 크기
 * @retval COMM_TX_SUCCESS 송신 성공 및 ACK 페이로드 수신 완료.
 * @retval COMM_TX_FAIL 최대 재전송 횟수 초과로 송신 실패.
 * @retval COMM_OK IRQ 이벤트가 발생하지 않은 상태.
 * @note NRF24 상태 레지스터를 읽어 TX_DS(송신 성공) 또는 MAX_RT(송신 실패) 비트를 확인한다.
 * 확인 후에는 해당 플래그를 클리어하여 다음 인터럽트를 준비한다.
 */
CommStatus_t CommHandler_CheckStatus(uint8_t* ack_payload, uint8_t len)
{
    // IRQ 플래그가 설정되지 않았으면 확인할 이벤트가 없으므로 종료한다.
    if (nrf_irq_flag == 0)
    {
        return COMM_OK;
    }

    nrf_irq_flag = 0; // 플래그를 다시 클리어한다.

    uint8_t status = nrf24_r_status();
    CommStatus_t result = COMM_OK;

    // TX_DS 비트가 1이면: 송신 성공 및 ACK 수신
    if (status & (1 << TX_DS))
    {
        // 수신 FIFO에 ACK 페이로드가 있는지 확인
        if (nrf24_data_available())
        {
            nrf24_receive(ack_payload, len); // ACK 페이로드 읽기
        }
        nrf24_clear_tx_ds(); // TX_DS 플래그 클리어
        result = COMM_TX_SUCCESS;
    }
    // MAX_RT 비트가 1이면: 최대 재전송 횟수 초과로 송신 실패
    else if (status & (1 << MAX_RT))
    {
        nrf24_flush_tx();      // TX FIFO를 비운다.
        nrf24_clear_max_rt();  // MAX_RT 플래그 클리어
        result = COMM_TX_FAIL;
    }

    return result;
}
