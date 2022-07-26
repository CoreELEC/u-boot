/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include <common.h>
#include "register.h"
#include "p_register.h"
#include "uart.h"
#include <task.h>
#include "soc.h"
#include "timer_source.h"
#include "clk.h"
#include "clk_util.h"
#include "vad_suspend.h"
#include "power_domain.h"
#include "mailbox-api.h"
#include "suspend.h"
#include "uart.h"

extern uint32_t get_reason_flag(void);

void set_time(uint32_t val)
{
#ifdef ARM_CPU
	REG32(SYSCTRL_SEC_TIMERE) = 0x0;
#else
	REG32(SYSCTRL_TIMERF) = val;
#endif
}

uint32_t get_time(void)
{
	uint64_t timerE = 0;

#ifdef ARM_CPU
	timerE = REG32(SYSCTRL_SEC_TIMERE);
#else
	timerE = REG32(SYSCTRL_TIMERF);
#endif
	return (timerE);
}

/*
 * use_clk_src
 * 0. osc_clk
 * 1. sys_clk (rtc_pll) clk_div1
 * 2. sys_clk (rtc_pll) clk_div2
 * 3. rtc_clk
 * Note:
 * 1. If select sys_clk, tick clk of 1us/10us/100us/1ms/xtal3 are all same
 * 2. If select rtc_clk, tick clk of 1us/10us/100us/1ms/xtal3 will actually
 *     be 1ms/10ms/100ms/1s/93.747us
 */

void alt_timebase(int use_clk_src)
{
	uint32_t clk_div = 0;

	if (use_clk_src == 0)
		//select osc_clk
		REG32(CLKCTRL_TIMEBASE_CTRL0) = 0x20018;
	else if (use_clk_src == 1) {
		//select sys_clk (rtc_pll) clk_div1
		//if sys_clk is 11.171MHz
		clk_div = 11;
		REG32(CLKCTRL_TIMEBASE_CTRL0) = (clk_div << 19) | (0x2aa << 6);
	} else if (use_clk_src == 2) {
		//select sys_clk (rtc_pll) clk_div2 when sys_clk < 256MHz
		//if sys_clk is 122.88MHz
		clk_div = 123;
		REG32(CLKCTRL_TIMEBASE_CTRL0) = (clk_div << 24) | (0x3ff << 6);
	} else {
		//select rtc clk
		// 32k/32 = 1k
		REG32(CLKCTRL_TIMEBASE_CTRL0) = 0x20020;
	}
}

/*
 * This is function for axi clk setting:
 * Includes axi_clk(AKA cpu_axi_clk/ACLK), axi_matrix, axi_sram
 *
 */
void set_axi_div_clk(int sel, int div)
{
	uint32_t control;
	uint32_t dyn_pre_mux;
	uint32_t dyn_div;

	// fclk_div4 = 500MHz
	dyn_pre_mux = sel;
	dyn_div = div; /* divide by 1 */

	control = REG32(CLKCTRL_AXI_CLK_CTRL0);
	if (control & (1 << 15)) { //default is preb, need use prea
		control =
			(control & ~(0x3fff)) | ((1 << 13) | (dyn_pre_mux << 10) | (dyn_div << 0));
		REG32(CLKCTRL_AXI_CLK_CTRL0) = control;
		control = control & ~(1 << 15);
		REG32(CLKCTRL_AXI_CLK_CTRL0) = control;
		udelay(25);
	} else {
		//preb
		control = (control & ~((0x3fff) << 16)) |
			  ((1 << 29) | (dyn_pre_mux << 26) | (dyn_div << 16));
		REG32(CLKCTRL_AXI_CLK_CTRL0) = control;
		control = control | (1 << 15);
		REG32(CLKCTRL_AXI_CLK_CTRL0) = control;
		udelay(25);
	}
}

/*
 * This is function for clk81 setting
 *
 */
void set_sys_div_clk(int sel, int div)
{
	uint32_t control;
	uint32_t dyn_pre_mux;
	uint32_t dyn_div;

	printf("Set sys clock to 167Mhz\n");

	// fclk_div4 = 500MHz
	dyn_pre_mux = sel;
	dyn_div = div; /* divide by 3 */

	control = REG32(CLKCTRL_SYS_CLK_CTRL0);

	if (control & (1 << 15)) { //default is preb, need use prea
		control =
			(control & ~(0x3fff)) | ((1 << 13) | (dyn_pre_mux << 10) | (dyn_div << 0));
		REG32(CLKCTRL_SYS_CLK_CTRL0) = control;
		control = control & ~(1 << 15);
		REG32(CLKCTRL_SYS_CLK_CTRL0) = control;
		udelay(25);
	} else {
		//preb
		control = (control & ~((0x3fff) << 16)) |
			  ((1 << 29) | (dyn_pre_mux << 26) | (dyn_div << 16));
		REG32(CLKCTRL_SYS_CLK_CTRL0) = control;
		control = control | (1 << 15);
		REG32(CLKCTRL_SYS_CLK_CTRL0) = control;
		udelay(25);
	}

	/* when bus switched from 24MHz to 166MHz,
	 * the clock tick used to generate OTP waveform
	 * needs 20us waiting time to switch to new clock tick
	 */
	udelay(25);
}
/*
 * clk_util_set_dsp_clk
 * freq_sel:
 *           0:800MHz  fclk_2p5
 *           1:400MHz  fclk_5
 *           2:500MHz  fclk_4
 *           3:666MHz  fclk_3
 *           4:333Mhz  fclk_3/2
 *           5:250Mhz  fclk_4/2
 *           6:200Mhz  fclk_5/2
 *           7:100Mhz  fclk_5/4
 *           8:24Mhz   oscin
 *           10:3Mhz    oscin/8
 *           others:400MHz  fclk_5
 *
 *.clk0            (cts_oscin_clk          ),
 *.clk1            (fclk_div2p5            ),
 *.clk2            (fclk_div3              ),
 *.clk3            (rtc_pll_clk            ),
 *.clk4            (hifi_pll_clk           ),
 *.clk5            (fclk_div4              ),
 *.clk6            (gp1_pll_clk            ),
 *.clk7            (cts_rtc_clk            ),
 */

/*  --------------------------------------------------
 *               clk_util_set_dsp_clk
 * --------------------------------------------------
 */

void clk_util_set_dsp_clk(uint32_t id, uint32_t freq_sel)
{
	uint32_t control;
	uint32_t clk_sel;
	uint32_t clk_div;
	uint32_t addr = CLKCTRL_DSPA_CLK_CTRL0;

	switch (id) {
	case 0:
		addr = CLKCTRL_DSPA_CLK_CTRL0;
		break;
	//default : addr = CLKCTRL_DSPB_CLK_CTRL0; break;
	default:
		printf("CLK_UTIL:Error, no dspb here\n");
		break;
	}

	//Make sure not busy from last setting and we currently match the last setting
	control = REG32(addr);

	switch (freq_sel) {
	case 0:
		clk_sel = 1;
		clk_div = 0;
		printf("CLK_UTIL:dsp[%d]:fclk2p5:800MHz\n", id);
		break;
	case 1:
		clk_sel = 1;
		clk_div = 1;
		printf("CLK_UTIL:dsp[%d]:fclk5:400MHz\n", id);
		break;
	case 2:
		clk_sel = 5;
		clk_div = 0;
		printf("CLK_UTIL:dsp[%d]:fclk4:500MHz\n", id);
		break;
	case 3:
		clk_sel = 2;
		clk_div = 0;
		printf("CLK_UTIL:dsp[%d]:fclk/3:667MHz\n", id);
		break;
	case 4:
		clk_sel = 2;
		clk_div = 1;
		printf("CLK_UTIL:dsp[%d]:fclk3/2:333MHz\n", id);
		break;
	case 5:
		clk_sel = 5;
		clk_div = 1;
		printf("CLK_UTIL:dsp[%d]:fclk4/2:250MHz\n", id);
		break;
	case 6:
		clk_sel = 1;
		clk_div = 3;
		printf("CLK_UTIL:dsp[%d]:fclk5/2:200MHz\n", id);
		break;
	case 7:
		clk_sel = 1;
		clk_div = 7;
		printf("CLK_UTIL:dsp[%d]:fclk5/4:100MHz\n", id);
		break;
	case 8:
		clk_sel = 0;
		clk_div = 0;
		printf("CLK_UTIL:dsp[%d]:oscin:24MHz\n", id);
		break;
	case 10:
		clk_sel = 0;
		clk_div = 7;
		printf("CLK_UTIL:dsp[%d]:oscin/8:3MHz\n", id);
		break;
	case 11:
		clk_sel = 3;
		clk_div = 0;
		printf("CLK_UTIL:dsp[%d]:rtc pll:122.88MHz\n", id);
		break;
	case 12:
		clk_sel = 7;
		clk_div = 0;
		printf("CLK_UTIL:dsp[%d]:rtc clk:24MHz\n", id);
		break;
	default:
		clk_sel = 1;
		clk_div = 7;
		printf("CLK_UTIL:dsp[%d]:fclk5:400MHz\n", id);
		break;
	}

	//if sync_mux ==1, sel mux 0
	if (control & (1 << 15))
		control = (control & ~((1 << 15) | (0x3ff << 0) | (0x7 << 10))) | (1 << 13) |
			  (1 << 29) | (clk_div << 0) | (clk_sel << 10);
	else
		control = (control & ~((1 << 15) | (0x3ff << 16) | (0x7 << 26))) | (1 << 13) |
			  (1 << 29) | (clk_div << 16) | (clk_sel << 26) | (1 << 15);

	REG32(addr) = control;
}

void disable_pll(int id)
{
	switch (id) {
	case PLL_SYS:
		REG32(ANACTRL_SYSPLL_CTRL0) = (1 << 29);
		break;
	case PLL_GP0:
		REG32(ANACTRL_GP0PLL_CTRL0) = (1 << 29);
		break;
	case PLL_GP1:
		REG32(ANACTRL_GP1PLL_CTRL0) = (1 << 29);
		break;
	case PLL_FIX:
		REG32(ANACTRL_FIXPLL_CTRL0) = (1 << 29);
		break;
	case PLL_HIFI:
		REG32(ANACTRL_HIFIPLL_CTRL0) = (1 << 29);
		break;
	case PLL_RTC:
		REG32(ANACTRL_RTCPLL_CTRL0) = (1 << 29);
		break;
	case PLL_DDR:
		REG32(AM_DDR_PLL_CNTL0) = (1 << 29);
		break;
	default:
		printf("Error: PLL is not in the list\n");
		break;
	}
}

int oscin_ctrl_reg;
void vCLK_resume(uint32_t st_f)
{
	int xIdx = 0;

	if ((!st_f) && (get_reason_flag() != VAD_WAKEUP)) {
		wakeup_dsp();
		vTaskDelay(pdMS_TO_TICKS(90));
		xIdx = WAKEUP_FROM_OTHER_KEY;
		xTransferMessageAsync(AODSPA_CHANNEL, MBX_CMD_SUSPEND_WITH_DSP, &xIdx, 4);
		clear_dsp_wakeup_trigger();
	}

	if (st_f) {
		/* switch osc_clk back*/
		REG32(CLKCTRL_SYSOSCIN_CTRL) = 1;
		REG32(CLKCTRL_OSCIN_CTRL) = oscin_ctrl_reg | (1 << 31);
		udelay(2000);
		/* switching tick timer (using osc_clk) */
		alt_timebase(0);

		set_sys_div_clk(0, 0); // osc_clk
		set_axi_div_clk(0, 0); // osc_clk
		vUartChangeBaudrate_resume(921600);
		udelay(1000);
		disable_pll(PLL_RTC);
		power_switch_to_wraper(PWR_ON);
		/* open mem_pd of srama and sramb */
		REG32(PWRCTRL_MEM_PD2) = 0x0;
	} else {

		xIdx = WAIT_SWITCH_TO_24MHZ;
		xTransferMessageAsync(AODSPA_CHANNEL, MBX_CMD_SUSPEND_WITH_DSP, &xIdx, 4);
		vTaskDelay(pdMS_TO_TICKS(90));
	}

	// In a55 boot code
	REG32(PWRCTRL_ACCESS_CTRL) = 0x0; // default access pwrctrl
}

void vCLK_suspend(uint32_t st_f)
{
	int xIdx = 0;

	printf("[AOCPU]: enter vCLK_suspend.\n");

	if (st_f) {
		/* close mem_pd of srama and sramb */
		REG32(PWRCTRL_MEM_PD2) = 0xfffffffc;
		/* poweroff wrapper */
		power_switch_to_wraper(PWR_OFF);
		udelay(2000);
		/* switch to RTC pll */
		set_sys_div_clk(6, 10); // rtc pll (11.171MHz)
		set_axi_div_clk(6, 0); // rtc pll (122.88MHz)
		alt_timebase(1); // 1us/10us/100us/1ms/xtal3 = 1 us tick  11.171/11

		udelay(2000);
		vUartChangeBaudrate_suspend(11171000, 921600);
		udelay(1000);

		/* power off osc_clk */
		REG32(CLKCTRL_SYSOSCIN_CTRL) = 0;
		oscin_ctrl_reg = REG32(CLKCTRL_OSCIN_CTRL);
		REG32(CLKCTRL_OSCIN_CTRL) = 0;
		printf("[AOCPU]: running at 11.171MHz, 24MHz osc clk power off.\n");

	} else {
		xIdx = WAIT_SWITCH_TO_RTC_PLL;
		xTransferMessageAsync(AODSPA_CHANNEL, MBX_CMD_SUSPEND_WITH_DSP, &xIdx, 4);
		vTaskDelay(pdMS_TO_TICKS(90));
		printf("[AOCPU]: running at 24MHz, 24MHz osc clk power off.\n");
	}
}
