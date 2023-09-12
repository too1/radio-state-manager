#include "radio_state_manager.h"
#include <zephyr/irq.h>
#include <hal/nrf_radio.h>

static struct rsm_state_mngr_t rsm_state_mngr = {0};

static void apply_radio_config(rsm_state_t *state)
{
	NRF_RADIO->SHORTS = state->radio_shorts;
	NRF_RADIO->INTENCLR = 0xFFFFFFFF;
	NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Msk;//(state->on_radio_disabled ? RADIO_INTENSET_DISABLED_Msk : 0) | (state->on_radio_end ? RADIO_INTENSET_END_Msk : 0);
}

static rsm_state_t *find_state_by_id(uint32_t id)
{
	if(id >= 1 && id < rsm_state_mngr.num_states) {
		return rsm_state_mngr.states[id];
	}
	return 0;
}

ISR_DIRECT_DECLARE(RADIO_IRQHandler)
{
	rsm_state_t *goto_state;
	rsm_state_t *current_state = rsm_state_mngr.current_state;
	if (nrf_radio_int_enable_check(NRF_RADIO, NRF_RADIO_INT_DISABLED_MASK) &&
	    nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

		if (current_state->on_radio_disabled) {
			current_state->on_radio_disabled(current_state);
		}
		goto_state = find_state_by_id(current_state->on_radio_disabled_goto_state);
		if (goto_state) {
			rsm_activate(goto_state);
		}
	}

	ISR_DIRECT_PM();
	return 1;
}

int rsm_init(void)
{
	rsm_state_mngr.num_states = 1;

	IRQ_DIRECT_CONNECT(RADIO_IRQn, 0, RADIO_IRQHandler, 0);
	//IRQ_DIRECT_CONNECT(ESB_TIMER_IRQ, CONFIG_ESB_EVENT_IRQ_PRIORITY, ESB_TIMER_IRQ_HANDLER, 0);
	irq_enable(RADIO_IRQn);

	return 0;
}

int rsm_add_state(rsm_state_t *newstate)
{
	if(rsm_state_mngr.num_states < RSM_MAX_STATES) {
		rsm_state_mngr.states[rsm_state_mngr.num_states] = newstate;
		rsm_state_mngr.num_states++;
		return 0;
	} else {
		return -ENOMEM;
	}
}

int rsm_activate(rsm_state_t *next_state)
{
	rsm_state_t *current_state = rsm_state_mngr.current_state;
	if (current_state != 0) {
		// Hacky fix to divert the state change when reaching the repeat limit 
		if(current_state->repeat >= current_state->repeat_limit && find_state_by_id(current_state->on_repeat_limit_goto_state) != 0) {
			current_state->repeat = 0;
			next_state = find_state_by_id(current_state->on_repeat_limit_goto_state);
		}

		if(current_state->on_state_end) {
			// Call the state end callback for the old state
			current_state->on_state_end(current_state, next_state);
		}
	}
	if (next_state != NULL) {
		if (next_state->on_state_start) {
			// Apply default state radio configuration
			apply_radio_config(next_state);
			// Call the state start callback for the new state
			next_state->on_state_start(next_state, current_state);
			// Increase the repeat counter of the state
			next_state->repeat++;

			rsm_state_mngr.current_state = next_state;
		}
	}
	return 0;
}
