#ifndef __RADIO_STATE_MANAGER_H
#define __RADIO_STATE_MANAGER_H

#include <zephyr/kernel.h>

#define RSM_MAX_STATES 32

enum rsm_state_end_reason {RSM_END_REASON_USER = 1, 
						   RSM_END_REASON_TIMEOUT,
						   RSM_END_REASON_RADIO_DIS,
						   RSM_END_REASON_REPEAT};

typedef struct _rsm_state_t rsm_state_t;

typedef void (*rsm_state_irq_t)(rsm_state_t *state, rsm_state_t *other);
typedef void (*rsm_radio_irq_t)(rsm_state_t *state);

struct _rsm_state_t {
	// State static parameters
	uint32_t id;
	char *name_str;

	// State runtime parameters
	uint32_t repeat;
	uint32_t repeat_limit;
	uint32_t timeout_us;

	// Automatic state change variables
	uint32_t on_radio_disabled_goto_state;
	uint32_t on_repeat_limit_goto_state;

	uint32_t end_reason;

	// Radio configuration parameters
	uint32_t radio_shorts;

	// State change callbacks
	rsm_state_irq_t on_state_start;
	rsm_state_irq_t on_state_end;
};

struct rsm_state_mngr_t {
	// State list
	rsm_state_t *states[RSM_MAX_STATES];
	int num_states; 
	int32_t current_state_id;
	rsm_state_t *current_state;
};

int rsm_init(void);

int rsm_add_state(rsm_state_t *newstate);

int rsm_activate(rsm_state_t *state);

#endif
