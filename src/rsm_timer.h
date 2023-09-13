#ifndef __RSM_TIMER_H
#define __RSM_TIMER_H

#include <zephyr/kernel.h>
#include "rsm_config.h"

#define LF_TIMER_CC_TIMEOUT 0

typedef void (*on_timeout_callback)(void);

void rsm_timer_init(void);

void rsm_timer_schedule_timeout(uint32_t time_us, on_timeout_callback callback);

#endif