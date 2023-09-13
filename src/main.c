#include <zephyr/kernel.h>
#include "rsm_core.h"

#define GPIOTE_CH_1 0
#define GPIOTE_CH_2 1

enum MY_STATES {STATE_IDLE = 1, STATE_TX, STATE_RX};

void state_tx_start(rsm_state_t *state, rsm_state_t *other);
void state_rx_start(rsm_state_t *state, rsm_state_t *other);
void state_idle_start(rsm_state_t *state, rsm_state_t *other);

static void gpiote_debug_init(int pin1, int pin2)
{
	NRF_GPIOTE->CONFIG[GPIOTE_CH_1] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
			pin1 << GPIOTE_CONFIG_PSEL_Pos;
	NRF_GPIOTE->CONFIG[GPIOTE_CH_2] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
			pin2 << GPIOTE_CONFIG_PSEL_Pos;
}

rsm_state_t m_state_idle = {
	.id = STATE_IDLE,
	.name_str = "Idle State",
	.radio_shorts = 0,
	.on_state_start = state_idle_start
};

rsm_state_t m_state_tx = {
	.id = STATE_TX,
	.name_str = "Radio transmit",
	.radio_shorts = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk | RADIO_SHORTS_DISABLED_RXEN_Msk,
	.on_state_start = state_tx_start,
	.on_radio_disabled_goto_state = STATE_RX,
	.task_state_start = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_1],
	.task_state_end = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_1]
};

rsm_state_t m_state_rx = {
	.id = STATE_TX,
	.name_str = "Radio receive",
	.radio_shorts = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk,
	.on_state_start = state_rx_start,
	.timeout_us = 500,
	.on_radio_disabled_goto_state = STATE_TX,
	.on_timeout_goto_state = STATE_TX,
	.on_repeat_limit_goto_state = STATE_IDLE, 
	.task_state_start = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_2],
	.task_state_end = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_2],
	.task_state_timeout = (uint32_t)&NRF_RADIO->TASKS_DISABLE
};

void state_tx_start(rsm_state_t *state, rsm_state_t *other)
{
	printk("TX %i\n", state->repeat);
	static uint8_t test_payload[64];
	for (int i = 0; i < sizeof(test_payload); i++) test_payload[i] = i + 0x10;
	if(state->repeat == 0) {
		NRF_RADIO->PACKETPTR = (uint32_t)test_payload;
		NRF_RADIO->PCNF0 =  0 << RADIO_PCNF0_LFLEN_Pos |
			0  << RADIO_PCNF0_S0LEN_Pos |
			0 << RADIO_PCNF0_S1LEN_Pos |
			1 << RADIO_PCNF0_PLEN_Pos; 
		NRF_RADIO->PCNF1 =  sizeof(test_payload) << RADIO_PCNF1_MAXLEN_Pos |
				sizeof(test_payload) << RADIO_PCNF1_STATLEN_Pos |
				4 << RADIO_PCNF1_BALEN_Pos |
				1 << RADIO_PCNF1_ENDIAN_Pos;
	}
	NRF_RADIO->TASKS_TXEN = 1;
}

void state_rx_start(rsm_state_t *state, rsm_state_t *other)
{
	printk("RX %i\n", state->repeat);
}

void state_idle_start(rsm_state_t *state, rsm_state_t *other)
{
	printk("Radio idle\n");
}

static void start_tx(int num)
{
	m_state_rx.repeat_limit = num;
	rsm_activate(&m_state_tx);
}

int main(void)
{    
	printk("Radio State Manager Test\n");

	gpiote_debug_init(33, 34);
	
	rsm_init();
	rsm_add_state(&m_state_idle);
	rsm_add_state(&m_state_tx);
	rsm_add_state(&m_state_rx);

	rsm_activate(&m_state_idle);

	while (1) {
		start_tx(10);
		k_msleep(2000);
	}

	return 0;
}
