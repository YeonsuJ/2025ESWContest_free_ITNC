// comm_handler.c

#include "comm_handler.h"
#include "NRF24.h"
#include "NRF24_reg_addresses.h"

static uint8_t tx_addr[5] = {0x45, 0x55, 0x67, 0x10, 0x21};
static volatile uint8_t nrf_irq_flag = 0;

void CommHandler_Init(void)
{
    csn_high();
    HAL_Delay(5);
    ce_low();
    nrf24_init();
    nrf24_stop_listen();
    nrf24_auto_ack_all(auto_ack);
    nrf24_en_ack_pld(enable);
    nrf24_dpl(disable);
    nrf24_set_crc(enable, _1byte);
    nrf24_tx_pwr(_0dbm);
    nrf24_data_rate(_2mbps);
    nrf24_set_channel(90);
    nrf24_set_addr_width(5);
    nrf24_auto_retr_delay(4);
    nrf24_auto_retr_limit(10);
    nrf24_open_tx_pipe(tx_addr);
    nrf24_open_rx_pipe(0, tx_addr);
    nrf24_pipe_pld_size(0, ACK_PAYLOAD_SIZE);
}

void CommHandler_IrqCallback(void)
{
    nrf_irq_flag = 1;
}

void CommHandler_Transmit(uint8_t* payload, uint8_t len)
{
    nrf24_transmit(payload, len);
}

CommStatus_t CommHandler_CheckStatus(uint8_t* ack_payload, uint8_t len)
{
    nrf_irq_flag = 0;
    uint8_t status = nrf24_r_status();
    CommStatus_t result = COMM_OK;
    if (status & (1 << TX_DS))
    {
        if (nrf24_data_available())
        {
            nrf24_receive(ack_payload, len);
        }
        nrf24_clear_tx_ds();
        result = COMM_TX_SUCCESS;
    }
    else if (status & (1 << MAX_RT))
    {
        nrf24_flush_tx();
        nrf24_clear_max_rt();
        result = COMM_TX_FAIL;
    }
    return result;
}
