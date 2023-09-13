#ifndef __RSM_CONFIG_H
#define __RSM_CONFIG_H

#define RSM_IRQ_PRI_HIGH 		0
#define RSM_IRQ_PRI_LOW			1

#define RSM_LF_TIMER 			NRF_RTC2
#define RSM_LF_TIMER_IRQn		RTC2_IRQn

#define RSM_EGU					NRF_EGU0
#define RSM_EGU_IRQn			SWI0_EGU0_IRQn

#endif