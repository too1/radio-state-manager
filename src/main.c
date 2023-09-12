#include <zephyr/kernel.h>
#include "radio_state_manager.h"

enum MY_STATES {STATE_IDLE = 1, STATE_TX, STATE_RX};

void state_tx_start(rsm_state_t *state, rsm_state_t *other);
void state_tx_radio_end(rsm_state_t *state);

rsm_state_t m_state_idle = {
	.id = STATE_IDLE,
	.name_str = "Idle State",
	.radio_shorts = 0
};

rsm_state_t m_state_tx = {
	.id = STATE_TX,
	.name_str = "Radio transmit",
	.radio_shorts = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk,
	.on_state_start = state_tx_start,
	.on_radio_disabled_goto_state = STATE_TX,
	.on_repeat_limit_goto_state = STATE_IDLE, 
};

void state_tx_start(rsm_state_t *state, rsm_state_t *other)
{
	printk("Starting TX...%i\n", state->repeat);
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

void state_tx_radio_end(rsm_state_t *state)
{

}
static void start_tx(int num)
{
	m_state_tx.repeat_limit = num;
	rsm_activate(&m_state_tx);
}

int main(void)
{    
	printk("Radio State Manager Test\n");

	rsm_init();
	rsm_add_state(&m_state_idle);
	rsm_add_state(&m_state_tx);

	rsm_activate(&m_state_idle);

	while (1) {
		start_tx(10);
		k_msleep(10000);
	}

	return 0;
}
