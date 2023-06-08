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
	} while (1 == (rd_reg(addr) & (1 << loc)))
#define wait_equal(addr, data)                                                                     \
	do {                                                                                       \
	} while (data != (rd_reg(addr)))

#define _udelay(tim) vTaskDelay(tim)
#define DDR_SUSPEND_MODE_MANUAL_TRIGGER_DFI_INIT_START 2

unsigned int g_nFlagASR;
unsigned int g_nAPDBack;
unsigned int g_nASRBack;
unsigned int g_nDMCReqBack; //for backup DMC req reg
unsigned int vco_ddr0;
unsigned int g_ndfinitcfg_value;
unsigned int dramfreq_input;
//unsigned int g_nAPDSet;
#define DMC_BASE_ADD    (0xff638400)
#define AML_PHY_BASE_ADD    (0xfe000000)

#define T3_ASR_CFG      (0x0003fe00)
#define P1_ASR_CFG      (0x00fffe00)

//#define DMC_DRAM_ASR_CTRL ((0x008d  << 2) + 0xff638400)//for example
void dram_suspend_asr(void)
{
	g_nFlagASR = rd_reg(DMC_DRAM_ASR_CTRL);
	wr_reg(DMC_DRAM_ASR_CTRL, P1_ASR_CFG);
}

void dram_resume_asr(void)
{
	wr_reg(DMC_DRAM_ASR_CTRL, g_nFlagASR);
}

#define AML_DDR_CHANNEL_ALL_IDLE   (0x0fffffff)

void vDDR_suspend(uint32_t st_f)
{
	//printf("aml log : DDR suspend...dummy\n");
	//return;

	(void)st_f;
	//unsigned int time_start, time_end;
	printf("Enter ddr suspend\n");

	g_nAPDBack = rd_reg(DMC_DRAM_APD_CTRL);
	wr_reg(DMC_DRAM_APD_CTRL, 0);
	g_nASRBack = rd_reg(DMC_DRAM_ASR_CTRL);
	wr_reg(DMC_DRAM_ASR_CTRL, 0);

	while (AML_DDR_CHANNEL_ALL_IDLE != rd_reg(DMC_CHAN_STS)) {
		printf("DMC_CHAN_STS: 0x%x\n", rd_reg(DMC_CHAN_STS));
		vTaskDelay(pdMS_TO_TICKS(100000));
	}

	/*backup and clear DMC req*/
	g_nDMCReqBack = rd_reg(DMC_REQ_CTRL);
	wr_reg(DMC_REQ_CTRL, 0); //bit0: A53.
	vTaskDelay(pdMS_TO_TICKS(1));

	/* suspend flow */
	/*PCTL to selfrefresh*/
	wr_reg(DMC_DRAM_SCFG, 2);

	//DMC_DRAM_STAT=3:DRAM_SLEEP;
	while (((rd_reg(DMC_DRAM_STAT)) & 0xf0) != 0x30) {
		//printf("DMC_DRAM_STAT22-ch0: 0x%x\n", rd_reg(DMC_DRAM_STAT));
		_udelay(1);
	}

	_udelay(1);
	g_ndfinitcfg_value = rd_reg(DMC_DRAM_DFIINITCFG);
	//g_ndfinitcfg_value = ((2 << 6) | (0 << 1) | (0 << 8) | (2 << 14) | (1 << 16) | (0 << 18));
	wr_reg(DMC_DRAM_DFIINITCFG, 0);
	wr_reg(DMC_DRAM_DFIINITCFG,
	(1 | (0 << 6) | (1 << 8) | (0 << 14) | (0 << 16) | (1 << 18)));
	_udelay(1);

	wait_clr(DMC_DRAM_DFIINITCFG, 31);
	_udelay(1);

	wr_reg((AML_PHY_BASE_ADD + 0x8000), (rd_reg(AML_PHY_BASE_ADD + 0x8000) &
		(~(0x1 << 2))));//turn off ddr0 odtp

	wr_reg((AML_PHY_BASE_ADD + 0x8004), (rd_reg(AML_PHY_BASE_ADD + 0x8004) &
		(~(0x1 << 2))));//turn off ddr0 odtn

	wr_reg((AML_PHY_BASE_ADD + 0x0150), (rd_reg(AML_PHY_BASE_ADD + 0x0150) &
		(~(0x1 << 16))));//disable phy p_ck_start_phase

	wr_reg((AML_PHY_BASE_ADD + 0xe024), 1);
	wr_reg((AML_PHY_BASE_ADD + 0xe044), 2);//turn off deskew pll

	wr_reg((AML_PHY_BASE_ADD + 0xe020), (rd_reg(AML_PHY_BASE_ADD + 0xe020) &
		(~(0x7 << 0))));//disable p_ck and dfi clk

	vco_ddr0 = rd_reg(AML_PHY_BASE_ADD + 0xe000);//save vco

	//force PLL reset,AM_DDR0_PLL_CNTL_OFFSET0=0xe000
	//disable master pll //bit 28 enable bit 29 reset
	wr_reg((AML_PHY_BASE_ADD + 0xe000), (vco_ddr0 | (0x1 << 29)));
	wr_reg((AML_PHY_BASE_ADD + 0xe000), (vco_ddr0 & (~(3 << 28))) | (0 << 28));
	printf("oytest1\n");
	/* print time consumption */
	//time_end = rd_reg(P_ISA_TIMERE);
	//printf("ddr suspend time: %dus\n", time_end - time_start);
	printf("\nddr suspend is done\n");
	//ddr_suspend_resume_test((1024<<20), 100, 3, 3, 0, 0);
	//ddr_suspend_resume_test((80<<20), 10000000, 0, 3, 0, 0);
}

static unsigned int pll_lock(unsigned int ddr_channel)
{
	unsigned int lock_cnt_ch0 = 100;
	unsigned int phy_base_add_temp;
//	if (ddr_channel == 1)
		phy_base_add_temp = AML_PHY_BASE_ADD;
//	else
//		phy_base_add_temp = 0xfc000000;

	wr_reg((phy_base_add_temp + 0xe000), (vco_ddr0 & (~(3 << 28))) | (0 << 28));
	_udelay(2);
	wr_reg((phy_base_add_temp + 0xe000), (vco_ddr0 & (~(3 << 28))) | (1 << 28));
	wr_reg((phy_base_add_temp + 0xe00c), rd_reg(phy_base_add_temp + 0xe00c) &
		(~(0x1 << 28)));  //reset pll lock;
	_udelay(2);//pll lock need 10us
	wr_reg((phy_base_add_temp + 0xe000), (vco_ddr0 & (~(3 << 28))) | (3 << 28));
	_udelay(2);//check lock
	wr_reg((phy_base_add_temp + 0xe020), 0xb0300007);

	dramfreq_input = 1176;
	//enable dfi clk to deskew pll ,pclk dfi clk gate
	wr_reg((phy_base_add_temp + 0xe044), 0x2);//reset deskew pll
	_udelay(1);
	wr_reg((phy_base_add_temp + 0xe048), 0x481dd03c);

	if (dramfreq_input < 900)
		wr_reg((phy_base_add_temp + 0xe04c), 0x009f824a);
		//<1.8 0x009f824a >1.8 0x009fc24a
	else
		wr_reg((phy_base_add_temp + 0xe04c), 0x009fc24a);
		//<1.8 0x009f824a >1.8 0x009fc24a

	wr_reg((phy_base_add_temp + 0xe044), 0x6);
	_udelay(1);
	wr_reg((phy_base_add_temp + 0xe044), 0x4);
	_udelay(1);
	wr_reg((phy_base_add_temp + 0xe044), 0xd);
	//bit3 deskew pll pclk to dll  enable
	_udelay(1);

	//poll_reg_phy(phy_base_add, 0x0000e044, 0, 0x80000000, 0x80000000);
	do {
		_udelay(1);//delay time need debug
	} while (((rd_reg(phy_base_add_temp + 0xe044) &
		(1 << 31)) != (1 << 31)) && (lock_cnt_ch0--));

	if (lock_cnt_ch0 == 0)
		return lock_cnt_ch0;

	_udelay(1);
	return lock_cnt_ch0;
}

void vDDR_resume(uint32_t st_f)
{
	//unsigned int time_start, time_end;
	unsigned int ret = 0;
	uint32_t last_time = timere_read();

	(void)st_f;
	printf("Enter ddr resume\n");

//return;

//time_start = rd_reg(P_ISA_TIMERE);
/* resume flow */
//	ret = pll_lock();
	if (!pll_lock(0)) {
		printf("ddr pll lock r1\n");
		//wr_reg(AM_DDR_PLL_CNTL3, rd_reg(AM_DDR_PLL_CNTL3) | (1 << 31));
		ret = pll_lock(0);
		if (!ret) {
			printf("ddr pll lock r2\n");
			//wr_reg(AM_DDR_PLL_CNTL6, 0x55540000);
			ret = pll_lock(0);
			if (!ret) {
				printf("ddr pll lock r2\n");
				while (1)
					;
			}
		}
	}
	wr_reg((AML_PHY_BASE_ADD + 0xe024), 0x53);
	_udelay(1);

	//enalbe all pclk dfi ctrl clk
	//recovery p_ck_start_phase
	wr_reg((AML_PHY_BASE_ADD + 0x0150), rd_reg(AML_PHY_BASE_ADD + 0x0150) &
		(~(0x1 << 16)));
	wr_reg((AML_PHY_BASE_ADD + 0x0150), rd_reg(AML_PHY_BASE_ADD + 0x0150) |
		(0x1 << 16));
	_udelay(1);
	//recovery odtp zq
	wr_reg((AML_PHY_BASE_ADD + 0x8000), rd_reg(AML_PHY_BASE_ADD + 0x8000) |
		(0x1 << 2));
	//recovery odtn zq
	wr_reg((AML_PHY_BASE_ADD + 0x8004), rd_reg(AML_PHY_BASE_ADD + 0x8004) |
		(0x1 << 2));
	_udelay(1);
	//printf("start lp2 exit init start\n");
	wr_reg(DMC_DRAM_DFIINITCFG,
	((0 << 6) | (1 << 8) | (0 << 14) | (0 << 16) | (1 << 18)));
	printf("start lp2 exit init start...\n");
	_udelay(1);
	/*wait for DFI init done*/
	wait_set(DMC_DRAM_DFIINITCFG, 31);
	_udelay(1);
	/*PCTL to access mode*/
	wr_reg(DMC_DRAM_SCFG, 4);
	//udelay(1);
	//DMC_DRAM_STAT=0:DRAM IDLE;=0x40:DRAM APD;=0x20:DRAM_ACCESS;
	while ((((rd_reg(DMC_DRAM_STAT)) & 0xf0) != 0) &&
		(((rd_reg(DMC_DRAM_STAT)) & 0xf0) != 0x20) &&
		(((rd_reg(DMC_DRAM_STAT)) & 0xf0) != 0x30) &&
		(((rd_reg(DMC_DRAM_STAT)) & 0xf0) != 0x40)) {
		printf("DMC_DRAM_STAT3-ch0: 0x%x\n", rd_reg(DMC_DRAM_STAT));
		_udelay(1);
	}
	wr_reg(DMC_REQ_CTRL, g_nDMCReqBack);

	wr_reg(DMC_DRAM_APD_CTRL, g_nAPDBack);

	wr_reg(DMC_DRAM_ASR_CTRL, g_nASRBack);
	_udelay(1);

	printf("ddr resume done %d us\n", timere_read() - last_time);
}

