/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * vad suspend header file
 */

#ifndef _VAD_SUSPEND_H_
#define _VAD_SUSPEND_H_

#ifdef __cplusplus
extern "C" {
#endif

/*use timerI to wakeup dsp FSM*/
static inline void wakeup_dsp(void)
{
	uint32_t value;
	//uint32_t time_out = 20;

	/*set alarm timer*/
	REG32(DSP_FSM_TRIGER_SRC) = 10;/*10us*/

	value = REG32(DSP_FSM_TRIGER_CTRL);
	value &= ~((1 << 7) | (0x3) | (1 << 6));
	value |= ((1 << 7) | (0 << 6) | (0x3));
	REG32(DSP_FSM_TRIGER_CTRL) = value;
	vTaskDelay(1);
}

static inline void clear_dsp_wakeup_trigger(void)
{
	REG32(DSP_FSM_TRIGER_SRC) = 0;
	REG32(DSP_FSM_TRIGER_CTRL) = 0;
}

#ifdef __cplusplus
}
#endif
#endif
