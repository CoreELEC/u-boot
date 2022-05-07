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
	} while (1 == (rd_reg(addr) & (1 << loc)))
#define wait_equal(addr, data)                                                                     \
	do {                                                                                       \
	} while (data != (rd_reg(addr)))

#define _udelay(tim) vTaskDelay(tim)
#define DDR_SUSPEND_MODE_MANUAL_TRIGGER_DFI_INIT_START 2

unsigned int g_nAPDSet;
void vDDR_suspend(uint32_t st_f)
{
	//printf("aml log : DDR suspend...dummy\n");
	//return;

	(void)st_f;
	//unsigned int time_start, time_end;
	printf("Enter ddr suspend\n");

	//return ;

	while (0xfffffff != rd_reg(DMC_CHAN_STS)) {
		printf("DMC_CHAN_STS: 0x%x\n", rd_reg(DMC_CHAN_STS));
		vTaskDelay(pdMS_TO_TICKS(100000));
	}

	//time_start = rd_reg(P_ISA_TIMERE);

	/* open DMC reg access for M3 */
	//apb_sec_ctrl = rd_reg(DDR_APB_SEC_CTRL);
	//wr_reg(DDR_APB_SEC_CTRL,0x91911);

	wr_reg(DMC_REQ_CTRL, 0); //bit0: A53.
	_udelay(1);

	/* suspend flow */
	while ((((rd_reg(DMC_DRAM_STAT)) & 0xf0) != 0) &&
	       (((rd_reg(DMC_DRAM_STAT)) & 0xf0) != 0x40)) {
		//	printf("DMC_DRAM_STAT11: 0x%x\n", rd_reg(DMC_DRAM_STAT));
		vTaskDelay(pdMS_TO_TICKS(1));
	}
#ifdef DDR_SUSPEND_MODE_DMC_TRIGGER_SUSPEND_1
	wr_reg(DMC_DRAM_ASR_CTRL,
	       (1 << 18)); //bit 18 will auto trigger dfi init start cmd when scfg set to value 2
	wr_reg(DMC_DRAM_SCFG, 2);
	while (((((rd_reg(DMC_DRAM_STAT)) >> 4) & 0xf) != 3)) {
		//printf("DMC_DRAM_STAT22: 0x%x\n", readl(DMC_DRAM_STAT));
		//_udelay(1);//do not add any delay,since use ao cpu maybe speed too slow
	}

#endif

#ifdef DDR_SUSPEND_MODE_MANUAL_TRIGGER_DFI_INIT_START
	wr_reg(DMC_DRAM_SCFG, 1);
	while (((((rd_reg(DMC_DRAM_STAT)) >> 4) & 0xf) != 1)) {
		//printf("DMC_DRAM_STAT22: 0x%x\n", readl(DMC_DRAM_STAT));
		//_udelay(1);//do not add any delay,since use ao cpu maybe speed too slow
	}

	wr_reg(DMC_DRAM_DFIINITCFG, (1 | (0 << 1) | (0 << 6) | (0 << 14) | (1 << 8)));
	vTaskDelay(pdMS_TO_TICKS(1));
	wait_clr(DMC_DRAM_DFIINITCFG, 31);
#endif //final version, wait_clr
	vTaskDelay(pdMS_TO_TICKS(3));
	wr_reg(AM_DDR_PLL_CNTL0, (rd_reg(AM_DDR_PLL_CNTL0) & (~(0xf << 28))) | (1 << 29));

	/* print time consumption */
	//time_end = rd_reg(P_ISA_TIMERE);
	//printf("ddr suspend time: %dus\n", time_end - time_start);
	printf("\nddr suspend is done\n");
	//ddr_suspend_resume_test((1024<<20), 100, 3, 3, 0, 0);
	//ddr_suspend_resume_test((80<<20), 10000000, 0, 3, 0, 0);
}

static unsigned int pll_lock(void)
{
	unsigned int lock_cnt = 100;

	do {
		wr_reg(AM_DDR_PLL_CNTL0, (rd_reg(AM_DDR_PLL_CNTL0) & (~(0xf << 28))) | (1 << 29));
		vTaskDelay(pdMS_TO_TICKS(1));
		wr_reg(AM_DDR_PLL_CNTL0, (rd_reg(AM_DDR_PLL_CNTL0) & (~(0x1 << 29))) | (1 << 28));
		vTaskDelay(pdMS_TO_TICKS(200));
	} while ((0 == ((rd_reg(AM_DDR_PLL_CNTL0) >> 31) & 0x1)) && (lock_cnt--));
	return lock_cnt;
}

void vDDR_resume(uint32_t st_f)
{
	//unsigned int time_start, time_end;
	unsigned int ret = 0;

	(void)st_f;
	printf("Enter ddr resume\n");

//return;

//time_start = rd_reg(P_ISA_TIMERE);
/* resume flow */
	ret = pll_lock();
	if (!ret) {
		printf("ddr pll lock r1\n");
		wr_reg(AM_DDR_PLL_CNTL3, rd_reg(AM_DDR_PLL_CNTL3) | (1 << 31));
		ret = pll_lock();
		if (!ret) {
			printf("ddr pll lock r2\n");
			wr_reg(AM_DDR_PLL_CNTL6, 0x55540000);
			ret = pll_lock();
			if (!ret) {
				printf("ddr pll lock r2\n");
				while (1)
					;
			}
		}
	}

#ifdef DDR_SUSPEND_MODE_DMC_TRIGGER_SUSPEND_1

#endif

#ifdef DDR_SUSPEND_MODE_MANUAL_TRIGGER_DFI_INIT_START
	wr_reg(DMC_DRAM_DFIINITCFG, (0 | (0 << 1) | (0 << 6) | (0 << 14) | (1 << 8)));
	vTaskDelay(pdMS_TO_TICKS(1));
	wait_set(DMC_DRAM_DFIINITCFG, 31);
	vTaskDelay(pdMS_TO_TICKS(100));
#endif
	wr_reg(DMC_DRAM_SCFG, 4);
	while (((((rd_reg(DMC_DRAM_STAT)) >> 4) & 0xf) != 2)) {
		//printf("DMC_DRAM_STAT22: 0x%x\n", readl(DMC_DRAM_STAT));
		//_udelay(1);//do not add any delay,since use ao cpu maybe speed too slow
	}

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
	printf("ddr resume done\n");
}
