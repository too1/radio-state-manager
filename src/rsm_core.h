#ifndef __RSM_CORE_H
#define __RSM_CORE_H

#include <zephyr/kernel.h>

#define RSM_MAX_STATES 32

#define RSM_RUN_TASK(a) *((uint32_t *)a) = 1

enum rsm_state_end_reason {RSM_END_REASON_USER = 1, 
						   RSM_END_REASON_TIMEOUT,
						   RSM_END_REASON_RADIO_DIS,
						   RSM_END_REASON_REPEAT};

typedef struct _rsm_state_t rsm_state_t;

typedef void (*rsm_state_irq_t)(rsm_state_t *state, rsm_state_t *other);
typedef void (*rsm_radio_irq_t)(rsm_state_t *state);

struct _rsm_state_t {
	// State static parameters
	const uint32_t id;
	const char *name_str;

	// State runtime parameters
	uint32_t repeat;
	uint32_t repeat_limit;
	uint32_t timeout_us;

	// Automatic state change variables
	const uint32_t on_radio_disabled_goto_state;
	const uint32_t on_repeat_limit_goto_state;
	const uint32_t on_timeout_goto_state;

	uint32_t end_reason;

	// Radio configuration parameters
	uint32_t radio_shorts;

	// State change callbacks
	const rsm_state_irq_t on_state_start;
	const rsm_state_irq_t on_state_end;

	// State task configurations
	const uint32_t task_state_start;
	const uint32_t task_state_end;
	const uint32_t task_state_timeout;
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
