#include "rsm_core.h"
#include "rsm_timer.h"
#include <zephyr/irq.h>
#include <hal/nrf_radio.h>

const char *rsm_state_str[] = {"0", "User", "Timeout", "R_Dis", "Repeat"};

#define EGU_CH_ANCHOR_TIME		0

static struct rsm_state_mngr_t rsm_state_mngr = {0};

static void execute_state_change(rsm_state_t *next_state, uint32_t reason);

static void apply_radio_config(rsm_state_t *state)
{
	NRF_RADIO->SHORTS = state->radio_shorts;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->INTENCLR = 0xFFFFFFFF;
	NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Msk;//(state->on_radio_disabled ? RADIO_INTENSET_DISABLED_Msk : 0) | (state->on_radio_end ? RADIO_INTENSET_END_Msk : 0);
}

static rsm_state_t *find_state_by_id(uint32_t id)
{
	if(id >= 1 && id < rsm_state_mngr.num_states) {
		return rsm_state_mngr.states[id];
	}
	printk("ERROR: invalid state id (%i)!\n", id);
	return 0;
}

static rsm_state_t *scheduled_timeout_state;
static void on_state_timeout(void)
{
	if (rsm_state_mngr.current_state->task_state_timeout) {
		RSM_RUN_TASK(rsm_state_mngr.current_state->task_state_timeout);
	}
	if (scheduled_timeout_state) {
		execute_state_change(scheduled_timeout_state, RSM_END_REASON_TIMEOUT);
	}	
}

static void execute_state_change(rsm_state_t *next_state, uint32_t reason)
{
	rsm_state_t *current_state = rsm_state_mngr.current_state;

	// State end behavior
	if (current_state != NULL) {
		//printk("SS %s to %s (%s)\n", current_state->name_str, next_state->name_str, rsm_state_str[reason]);
		// Check if we have reached the repeat limit, and modify the next_state and reason in this case
		if (current_state->repeat_limit && current_state->repeat >= current_state->repeat_limit 
			&& current_state->on_repeat_limit_goto_state) {
			current_state->repeat = 0;
			next_state = find_state_by_id(current_state->on_repeat_limit_goto_state);
			reason = RSM_END_REASON_REPEAT;
		}

		// Update the end_reason value for the current state		
		current_state->end_reason = reason;

		// If enabled, call the state end callback for the old state
		if(current_state->on_state_end) {
			current_state->on_state_end(current_state, next_state);
		}
		// If enabled, trigger the end stask for the old state
		if (current_state->task_state_end) {
			RSM_RUN_TASK(current_state->task_state_end);
		}
	}

	// State start behavior
	if (next_state != NULL) {
		// If a timeout is defined, schedule the timeout callback
		if (next_state->timeout_us) {//} && next_state->on_timeout_goto_state) {
			scheduled_timeout_state = next_state->on_timeout_goto_state ? find_state_by_id(next_state->on_timeout_goto_state) : NULL;
			rsm_timer_schedule_timeout(next_state->timeout_us, on_state_timeout);
		}

		// Apply default state radio configuration
		apply_radio_config(next_state);
		
		// If enabled, trigger the start stask for the new state
		if (next_state->task_state_start) {
			RSM_RUN_TASK(next_state->task_state_start);
		}
		// If enabled, call the state start callback for the new state
		if (next_state->on_state_start) {
			next_state->on_state_start(next_state, current_state);
		}
		
		// Increase the repeat counter of the state
		next_state->repeat++;

		rsm_state_mngr.current_state = next_state;
	}
}

ISR_DIRECT_DECLARE(RADIO_IRQHandler)
{
	NRF_P1->OUTSET = (1 << 5);
	rsm_state_t *goto_state;
	rsm_state_t *current_state = rsm_state_mngr.current_state;
	if (nrf_radio_int_enable_check(NRF_RADIO, NRF_RADIO_INT_DISABLED_MASK) &&
	    nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

		// Otherwise see if there is a regular disabled callback stored
		if (current_state->on_radio_disabled_goto_state) {
			goto_state = find_state_by_id(current_state->on_radio_disabled_goto_state);
			execute_state_change(goto_state, RSM_END_REASON_RADIO_DIS);
		}
	}
	NRF_P1->OUTCLR = (1 << 5);
	ISR_DIRECT_PM();
	return 1;
}

ISR_DIRECT_DECLARE(RSM_EGU_IRQHandler)
{
	if (RSM_EGU->EVENTS_TRIGGERED[EGU_CH_ANCHOR_TIME]) {
		RSM_EGU->EVENTS_TRIGGERED[EGU_CH_ANCHOR_TIME] = 0;
		rsm_timer_set_anchor();
	}

	ISR_DIRECT_PM();
	return 1;
}

int rsm_init(void)
{
	rsm_state_mngr.num_states = 1;

	IRQ_DIRECT_CONNECT(RADIO_IRQn, RSM_IRQ_PRI_HIGH, RADIO_IRQHandler, 0);
	irq_enable(RADIO_IRQn);

	IRQ_DIRECT_CONNECT(RSM_EGU_IRQn, RSM_IRQ_PRI_HIGH, RSM_EGU_IRQHandler, 0);
	irq_enable(RSM_EGU_IRQn);

	rsm_timer_init();

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
	execute_state_change(next_state, RSM_END_REASON_USER);
	return 0;
}
