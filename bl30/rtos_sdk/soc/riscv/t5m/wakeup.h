/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __WAKEUP_H__
#define __WAKEUP_H__

#define TIMER_CLK_IDX	3// 0:cts_sys_clk 1:1us 2:10us 3:100us
#define WAKEUP_TIMER_CNT	1000 //Delay : WAKEUP_TIMER_CNT * TIMER_CLK_IDX

/* Calculate the actual value, unit ms */
#ifndef TIMER_CLK_IDX
#error "Not define TIMER_CLK_IDX macro..\n"
#else
	#if !((TIMER_CLK_IDX >= 0) && (TIMER_CLK_IDX <= 3))
	#error "Set invalid value for TIMER"
	#else
		#if (TIMER_CLK_IDX == 0)
		#error "Not support!"
		#elif (TIMER_CLK_IDX == 1)
		#define SYSTEM_DELAY_VAL	(WAKEUP_TIMER_CNT / 1000)
		#elif (TIMER_CLK_IDX == 2)
		#define SYSTEM_DELAY_VAL	(WAKEUP_TIMER_CNT / 1000 * 10)
		#elif (TIMER_CLK_IDX == 3)
		#define SYSTEM_DELAY_VAL	(WAKEUP_TIMER_CNT / 1000 * 100)
		#endif
	#endif //!((TIMER_CLK_IDX >= 0) && (TIMER_CLK_IDX <=3))
#endif // TIMER_CLK_IDX

/*use timerB to wakeup AP FSM*/
static inline void wakeup_ap(void)
{
	uint32_t value;
	//uint32_t time_out = 20;

	/*set alarm timer*/
	REG32(FSM_TRIGER_SRC) = WAKEUP_TIMER_CNT; /*1ms*/

	value = REG32(FSM_TRIGER_CTRL);
	value &= ~((1 << 7) | (0x3) | (1 << 6));
	value |= ((1 << 7) | (0 << 6) | (TIMER_CLK_IDX));
	REG32(FSM_TRIGER_CTRL) = value;
	vTaskDelay(pdMS_TO_TICKS(SYSTEM_DELAY_VAL) + 1);
}

static inline void clear_wakeup_trigger(void)
{
	REG32(FSM_TRIGER_SRC) = 0;
	REG32(FSM_TRIGER_CTRL) = 0;
}

static inline void watchdog_reset_system(void)
{
	int i = 0;

	printf("enter %s\n", __func__);
	while (1) {
		REG32(RESETCTRL_WATCHDOG_CTRL0) = 1 << 27 | 0 << 18;
		/* Decive GCC for waiting some cycles */
		for (i = 0; i < 100; i++)
			REG32(RESETCTRL_WATCHDOG_CTRL0);
	}
}
#endif
