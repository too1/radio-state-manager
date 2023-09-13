#include "rsm_timer.h"
#include <zephyr/irq.h>

static on_timeout_callback scheduled_timeout_callback = 0;

ISR_DIRECT_DECLARE(LF_TIMER_IRQHandler)
{
	if (RSM_LF_TIMER->EVENTS_COMPARE[LF_TIMER_CC_TIMEOUT]) {
		RSM_LF_TIMER->EVENTS_COMPARE[LF_TIMER_CC_TIMEOUT] = 0;

		// If a timeout callback is scheduled, activate it
		if (scheduled_timeout_callback) {
			scheduled_timeout_callback();
		}

		scheduled_timeout_callback = 0;
		RSM_LF_TIMER->INTENCLR = RTC_INTENCLR_COMPARE0_Msk << LF_TIMER_CC_TIMEOUT;
	}

	ISR_DIRECT_PM();
	return 1;
}

void rsm_timer_init(void)
{
	RSM_LF_TIMER->PRESCALER = 0;

	IRQ_DIRECT_CONNECT(RSM_LF_TIMER_IRQn, RSM_IRQ_PRI_HIGH, LF_TIMER_IRQHandler, 0);
	irq_enable(RSM_LF_TIMER_IRQn);

	RSM_LF_TIMER->TASKS_CLEAR = 1;
	RSM_LF_TIMER->TASKS_START = 1;
}

void rsm_timer_schedule_timeout(uint32_t time_us, on_timeout_callback callback)
{
	uint32_t current_time = RSM_LF_TIMER->COUNTER;
	RSM_LF_TIMER->CC[LF_TIMER_CC_TIMEOUT] = current_time + (time_us + 31) / 32;
	scheduled_timeout_callback = callback;
	RSM_LF_TIMER->EVTENSET = RTC_EVTENSET_COMPARE0_Msk << LF_TIMER_CC_TIMEOUT;
	RSM_LF_TIMER->INTENSET = RTC_INTENSET_COMPARE0_Msk << LF_TIMER_CC_TIMEOUT;
}