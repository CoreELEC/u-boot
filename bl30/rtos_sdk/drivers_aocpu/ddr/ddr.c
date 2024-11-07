/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "ddr.h"
#include "common.h"
#include "register.h"
#include "FreeRTOS.h"
#include "task.h"
#include "soc.h"
#include "suspend_debug.h"

#define timere_read()	REG32(TIMERE_LOW_REG)
/* io defines */
#define wr_reg(addr, val) ((*((volatile uint32_t *)(addr))) = (val))
#define rd_reg(addr) (*((volatile uint32_t *)(addr)))

/*clear [mask] 0 bits in [addr], set these 0 bits with [value] corresponding bits*/
#define modify_reg(addr, value, mask) wr_reg(addr, ((rd_reg(addr) & (mask)) | (value)))
#define wait_set(addr, loc)                                                                        \
	do {                                                                                       \
	} while (0 == (rd_reg(addr) & (1 << loc)))
#define wait_clr(addr, loc)                                                                        \
	do {                                                                                       \
	} while (0x80000000 == (rd_reg(addr) & (1 << loc)))
#define wait_equal(addr, data)                                                                     \
	do {                                                                                       \
	} while (data != (rd_reg(addr)))

//#define _udelay(tim) vTaskDelay(tim)
void _udelay(uint32_t delay_us)
{
	unsigned int time1;
	unsigned int time2;

	time1 = timere_read();
	do {
		time2 = timere_read();
	} while ((time2 - time1) < delay_us);
}

#define udelay(tim)  _udelay(tim)
//unsigned int g_nAPDSet = 0;
#define DDR_SUSPEND_MODE_MANUAL_TRIGGER_DFI_INIT_START 2
unsigned int	test_loops;
unsigned int apd_value;

void vDDR_suspend(uint32_t st_f)
{
	printf("aml log : DDR suspend...test2\n");
	apd_value = rd_reg(DMC_DRAM_APD_CTRL);
	wr_reg(DMC_DRAM_APD_CTRL, 0);
	//ddr_suspend_resume_test((1024<<20), 5000, 100, 100, 0, 0);
	//return;
	//st_f = st_f;
	//unsigned int time_start, time_end;
	printf("Enter ddr suspend test_loops = %d\n", test_loops);
	//if (test_loops == 0) {
	//	dmc_ddr_test(0x10000000, 1, 0, 0, 0x30000000, 1, 0);
		//test_loops++;
	//}
	test_loops++;
	//some address will be protect by bl30
	//return ;

	while (0xfffffff != rd_reg(DMC_CHAN_STS)) {
		printf("DMC_CHAN_STS: 0x%x\n", rd_reg(DMC_CHAN_STS));
#if BL30_SUSPEND_DEBUG_EN
/* ddr suspend debug : print vio reg and clear */
		dmc_status_print_clear();
#endif
		//vTaskDelay(pdMS_TO_TICKS(100000));
		udelay(1);
	}

#if BL30_SUSPEND_DEBUG_EN
/* ddr suspend debug : print vio before REQ_CTRL disabled*/
		dmc_status_disable_print();
#endif

	//time_start = rd_reg(P_ISA_TIMERE);

	/* open DMC reg access for M3 */
	//apb_sec_ctrl = rd_reg(DDR_APB_SEC_CTRL);
	//wr_reg(DDR_APB_SEC_CTRL,0x91911);
	printf("DMC_DRAM_MCFG: 0x%x\n", rd_reg(DMC_DRAM_MCFG));
	wr_reg(DMC_REQ_CTRL, 0); //bit0: A53.
	udelay(1);
//return;
	/* suspend flow */
	//while ((((rd_reg(DMC_DRAM_STAT))&0xf0) != 0) && ((rd_reg(DMC_DRAM_STAT))&0xf0) != 0x40) {
	//	printf("DMC_DRAM_STAT11: 0x%x\n", rd_reg(DMC_DRAM_STAT));
	//	//vTaskDelay(pdMS_TO_TICKS(1));
	//	udelay(1);
	//}
#ifdef DDR_SUSPEND_MODE_DMC_TRIGGER_SUSPEND_1
	wr_reg(DMC_DRAM_ASR_CTRL, (1 << 18));
	//bit 18 will auto trigger dfi init start cmd when scfg set to value 2
	wr_reg(DMC_DRAM_SCFG, 2);

	while (((((rd_reg(DMC_DRAM_STAT))>>4)&0xf) != 3)) {
		//printf("DMC_DRAM_STAT22: 0x%x\n", readl(DMC_DRAM_STAT));
		//_udelay(1);//do not add any delay,since use ao cpu maybe speed too slow
	}

#endif

//return;
#ifdef DDR_SUSPEND_MODE_MANUAL_TRIGGER_DFI_INIT_START
	//wr_reg(DMC_DRAM_SCFG, 2);
	//wr_reg(DMC_DRAM_MCFG, (1 << 9) | rd_reg(DMC_DRAM_MCFG));
	//while (((((rd_reg(DMC_DRAM_STAT))>>4)&0xf) != 3)) {
		//printf("DMC_DRAM_STAT22: 0x%x\n", readl(DMC_DRAM_STAT));
		//_udelay(1);//do not add any delay,since use ao cpu maybe speed too slow
	//}
	wr_reg(DMC_DRAM_SCFG, 1);
	//wr_reg(DMC_DRAM_MCFG, (1 << 9) | rd_reg(DMC_DRAM_MCFG));
	while (((((rd_reg(DMC_DRAM_STAT))>>4)&0xf) != 1)) {
		//printf("DMC_DRAM_STAT22: 0x%x\n", readl(DMC_DRAM_STAT));
		//_udelay(1);//do not add any delay,since use ao cpu maybe speed too slow
	}

	wr_reg(DMC_DRAM_DFIINITCFG, (1 | (0 << 1) | (0 << 6) | (0 << 14) | (1 << 8)));
	//vTaskDelay(pdMS_TO_TICKS(1));
	udelay(1);
	wait_clr(DMC_DRAM_DFIINITCFG, 31);
#endif//final version, wait_clr
	//vTaskDelay(pdMS_TO_TICKS(3));
	udelay(3);
	//return;
	wr_reg(AM_DDR_PLL_CNTL0, (rd_reg(AM_DDR_PLL_CNTL0) & (~(0xf << 28))) | (1 << 29));

	/* print time consumption */
	//time_end = rd_reg(P_ISA_TIMERE);
	//printf("ddr suspend time: %dus\n", time_end - time_start);
	printf("\nddr suspend is done\n");
	//ddr_suspend_resume_test((1024<<20), 100, 3, 3, 0, 0);
	//ddr_suspend_resume_test((80<<20), 10000000, 0, 3, 0, 0);
}

//#if 1
static unsigned int pll_lock(void)
{
	unsigned int lock_cnt = 1000;

	//wr_reg(AM_DDR_PLL_CNTL0, (rd_reg(AM_DDR_PLL_CNTL0) & (~(0xf << 28))) | (1 << 29));
	//vTaskDelay(pdMS_TO_TICKS(1));
	//udelay(100);
	//wr_reg(AM_DDR_PLL_CNTL0, (rd_reg(AM_DDR_PLL_CNTL0) & (~(0x1 << 29))) | (1 << 28));
	do {
		//vTaskDelay(pdMS_TO_TICKS(1));
		wr_reg(AM_DDR_PLL_CNTL0, (rd_reg(AM_DDR_PLL_CNTL0) & (~(0xf << 28))) | (1 << 29));
		//vTaskDelay(pdMS_TO_TICKS(1));
		udelay(1);
		wr_reg(AM_DDR_PLL_CNTL0, (rd_reg(AM_DDR_PLL_CNTL0) & (~(0x1 << 29))) | (1 << 28));
		udelay(200);
	} while ((0 == ((rd_reg(AM_DDR_PLL_CNTL0) >> 31) & 0x1)) && (lock_cnt--));
	udelay(1000);
	return lock_cnt;

}
//#endif

void vDDR_resume(uint32_t st_f)
{
	//unsigned int time_start, time_end;
	if (st_f)
		return;
	//st_f = st_f;
	_udelay(30000);
	printf("Enter ddr resume\n");
	uint32_t last_time = timere_read();
	unsigned int cost_time = 0;

	udelay(100);
	cost_time = timere_read() - last_time;
	printf("ddr test delay 100us = %d us\n", cost_time);
	//return;

	//time_start = rd_reg(P_ISA_TIMERE);
	/* resume flow */
//#if 1
	unsigned int ret = 0;
	//unsigned int cost_time = 0;
	last_time = timere_read();
	ret = pll_lock();
	if (!ret) {
		printf("ddr pll lock r1\n");
		wr_reg(AM_DDR_PLL_CNTL3, rd_reg(AM_DDR_PLL_CNTL3)|(1<<31));
		ret = pll_lock();
		if (!ret) {
			printf("ddr pll lock r2\n");
			wr_reg(AM_DDR_PLL_CNTL6, 0x55540000);
			ret = pll_lock();
			if (!ret) {
				printf("ddr pll lock r2\n");
				do {
				} while (1);
			}
		}
	}
	cost_time = timere_read() - last_time;
	printf("ddr resume pll done %d us\n", cost_time);
//#endif
//#if 1
#ifdef DDR_SUSPEND_MODE_DMC_TRIGGER_SUSPEND_1

#endif

#ifdef DDR_SUSPEND_MODE_MANUAL_TRIGGER_DFI_INIT_START
	wr_reg(DMC_DRAM_DFIINITCFG, (0 | (0 << 1) | (0 << 6) | (0 << 14) | (1 << 8)));
	//vTaskDelay(pdMS_TO_TICKS(1));
	//printf("aml log : DDR suspend...dfi\n");
	udelay(1);
	wait_set(DMC_DRAM_DFIINITCFG, 31);
	//vTaskDelay(pdMS_TO_TICKS(1));
	udelay(1);
#endif
//#endif
	wr_reg(DMC_DRAM_SCFG, 4);
	while (((((rd_reg(DMC_DRAM_STAT))>>4)&0xf) != 2)) {
		//printf("DMC_DRAM_STAT22: 0x%x\n", readl(DMC_DRAM_STAT));
		//_udelay(1);//do not add any delay,since use ao cpu maybe speed too slow
	}
	//vTaskDelay(pdMS_TO_TICKS(1));
	_udelay(10000);//at least a refresh time to update zq dll
	last_time = timere_read();
#if BL30_SUSPEND_DEBUG_EN
/* ddr suspend debug : print vio reg and channel status before resume */
	show_dmc_port_status();
#endif

	wr_reg(DMC_REQ_CTRL, 0xffffffff);
	//wr_reg(DDR_APB_SEC_CTRL, apb_sec_ctrl);
	/* print time consumption */
	//time_end = readl(P_ISA_TIMERE);
	//printf("ddr resume time: %dus\n", time_end - time_start);
	//	unsigned int ddr_bist_test_error = 0;
	//ddr_bist_test_error = dmc_ddr_test(dram_base, 0, 1, 1, test_size, 1, 0) + ddr_bist_test_error;
	//ddr_bist_test_error = dmc_ddr_test(0, 0, 1, 1, (80<<20), 1, 0) + ddr_bist_test_error;
	//printf("ddr_bist_test_error = %d\n", ddr_bist_test_error);
	//wr_reg(0xfe002440, 2);
//	wr_reg(0xfe002440, 0);
//	_udelay(300);
	//ddr_bist_test_error = dmc_ddr_test(0, 1, 0, 0, (1<<20), 1, 0) + ddr_bist_test_error;
	//ddr_bist_test_error = dmc_ddr_test(0, 0, 1, 1, (1<<20), 1, 0) + ddr_bist_test_error;
//	printf("ddr_bist_test_error = %d\n", ddr_bist_test_error);
	//ddr_suspend_resume_test((1<<20), 2, 1, 3, 0, 0);
	//ddr_suspend_resume_test((1<<20), 0, 1, 3, 0, 0);
	//_udelay(300);
//	wr_reg(DMC_REQ_CTRL, 0xffffffff);
	cost_time = timere_read() - last_time;
	printf("ddr resume done %d us\n", cost_time);

	//unsigned int ddr_bist_test_error = 0;
	//ddr_bist_test_error = dmc_ddr_test(0x10000000, 0, 1, 1, 0x30000000, 1, 0);
	//if (ddr_bist_test_error) {
	//	ddr_bist_test_error =
	//	dmc_ddr_test(0x10000000, 0, 1, 1, 0x30000000, 1, 0) + ddr_bist_test_error;
	//	ddr_bist_test_error =
	//	dmc_ddr_test(0x10000000, 0, 1, 1, 0x30000000, 1, 0) + ddr_bist_test_error;
	//}
	//printf( "dmc full test result = %d\n", ddr_bist_test_error);

	wr_reg(DMC_DRAM_APD_CTRL, apd_value);
	wr_reg(DMC_DRAM_ASR_CTRL, (0 << 18));
}
