/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _CLK_H_
#define _CLK_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PLL_SYS,
	PLL_GP0,
	PLL_GP1,
	PLL_FIX,
	PLL_HIFI,
	PLL_RTC,
	PLL_DDR
} PLL_TYPE;

void disable_pll(int id);
void set_time(uint32_t val);
uint32_t get_time(void);
void alt_timebase(int use_clk_src);
void set_sys_div_clk(int sel, int div);
void set_axi_div_clk(int sel, int div);
void clk_util_set_dsp_clk(uint32_t id, uint32_t freq_sel);

#ifdef __cplusplus
}
#endif
#endif
