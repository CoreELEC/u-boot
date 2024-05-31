/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include "FreeRTOS.h"
#include "suspend.h"
#include "task.h"
#include "gpio.h"
#include "common.h"

#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include "hdmirx_wake.h"
#include "timer_source.h"

#define GPIO_HDMI_RX1_POWER	GPIOW_1
#define GPIO_HDMI_RX2_POWER	GPIOW_9
#define GPIO_HDMI_RX3_POWER	GPIOW_5
#define GPIO_HDMI_RX4_POWER	GPIOW_13

static void hdmirx_IRQHandle(void)
{
	uint32_t buf[4] = {0};

	vDisableGpioIRQ(GPIO_HDMI_RX1_POWER);
	vDisableGpioIRQ(GPIO_HDMI_RX2_POWER);
	vDisableGpioIRQ(GPIO_HDMI_RX3_POWER);
	vDisableGpioIRQ(GPIO_HDMI_RX4_POWER);
	if (xGpioGetValue(GPIO_HDMI_RX1_POWER) ||
		xGpioGetValue(GPIO_HDMI_RX2_POWER) ||
		xGpioGetValue(GPIO_HDMI_RX3_POWER) ||
		xGpioGetValue(GPIO_HDMI_RX4_POWER)) {
		buf[0] = HDMI_RX_WAKEUP;
		INFO("hdmirx_5v A/B/C/D input lvl: %d : %d : %d : %d",
			xGpioGetValue(GPIO_HDMI_RX1_POWER),
			xGpioGetValue(GPIO_HDMI_RX2_POWER),
			xGpioGetValue(GPIO_HDMI_RX3_POWER),
			xGpioGetValue(GPIO_HDMI_RX4_POWER));
	}
	STR_Wakeup_src_Queue_Send_FromISR(buf);
}

void hdmirx_GpioIRQRegister(void)
{
	/* clear pinmux */
	xPinmuxSet(GPIO_HDMI_RX1_POWER, PIN_FUNC0);
	xPinmuxSet(GPIO_HDMI_RX2_POWER, PIN_FUNC0);
	xPinmuxSet(GPIO_HDMI_RX3_POWER, PIN_FUNC0);
	xPinmuxSet(GPIO_HDMI_RX4_POWER, PIN_FUNC0);

	xGpioSetDir(GPIO_HDMI_RX1_POWER, GPIO_DIR_IN);
	xGpioSetDir(GPIO_HDMI_RX2_POWER, GPIO_DIR_IN);
	xGpioSetDir(GPIO_HDMI_RX3_POWER, GPIO_DIR_IN);
	xGpioSetDir(GPIO_HDMI_RX4_POWER, GPIO_DIR_IN);

	INFO("hdmirx_5v A/B/C/D input lvl: %d : %d : %d : %d",
		xGpioGetValue(GPIO_HDMI_RX1_POWER),
		xGpioGetValue(GPIO_HDMI_RX2_POWER),
		xGpioGetValue(GPIO_HDMI_RX3_POWER),
		xGpioGetValue(GPIO_HDMI_RX4_POWER));

	xRequestGpioIRQ(GPIO_HDMI_RX1_POWER, hdmirx_IRQHandle, IRQF_TRIGGER_RISING);
	xRequestGpioIRQ(GPIO_HDMI_RX2_POWER, hdmirx_IRQHandle, IRQF_TRIGGER_RISING);
	xRequestGpioIRQ(GPIO_HDMI_RX3_POWER, hdmirx_IRQHandle, IRQF_TRIGGER_RISING);
	xRequestGpioIRQ(GPIO_HDMI_RX4_POWER, hdmirx_IRQHandle, IRQF_TRIGGER_RISING);
}

void hdmirx_GpioIRQFree(void)
{
	vFreeGpioIRQ(GPIO_HDMI_RX1_POWER);
	vFreeGpioIRQ(GPIO_HDMI_RX2_POWER);
	vFreeGpioIRQ(GPIO_HDMI_RX3_POWER);
	vFreeGpioIRQ(GPIO_HDMI_RX4_POWER);
	/* recovery pinmux */
	xPinmuxSet(GPIO_HDMI_RX1_POWER, PIN_FUNC1);
	xPinmuxSet(GPIO_HDMI_RX2_POWER, PIN_FUNC1);
	xPinmuxSet(GPIO_HDMI_RX3_POWER, PIN_FUNC1);
	xPinmuxSet(GPIO_HDMI_RX4_POWER, PIN_FUNC1);
}

void rx_wr_reg_b(unsigned long reg_addr, unsigned int val)
{
	REG32(reg_addr) = val;
}

unsigned int rx_rd_reg_b(unsigned long reg_addr)
{
	return REG32(reg_addr);
}

void hdmirx_wr_single_top(unsigned long addr, unsigned long data, unsigned char port)
{
	unsigned long dev_offset = 0;

	dev_offset = HDMIRX_SINGLE_TOP_BASE_T3X + HDMIRX_SINGLE_TOP_OFFSET_T3X * port;
	rx_wr_reg_b(dev_offset + (addr << 2), data);
}

unsigned int hdmirx_rd_single_top(unsigned long reg_addr, unsigned char port)
{
	unsigned long dev_offset = 0;

	dev_offset = HDMIRX_SINGLE_TOP_BASE_T3X + HDMIRX_SINGLE_TOP_OFFSET_T3X * port;
	return rx_rd_reg_b(dev_offset + (reg_addr << 2));
}

unsigned int hdmirx_rd_common_top(unsigned long reg_addr)
{
	unsigned long dev_offset = 0;

	dev_offset = HDMIRX_COMMON_TOP_BASE_T3X;
	return rx_rd_reg_b(dev_offset + (reg_addr << 2));
}

void hdmirx_wr_common_top(unsigned long addr, unsigned long data)
{
	unsigned long dev_offset = 0;

	dev_offset = HDMIRX_COMMON_TOP_BASE_T3X;
	rx_wr_reg_b(dev_offset + (addr << 2), data);
}

unsigned long hdmirx_rd_amlphy_t3x(unsigned long addr, unsigned char port)
{
	ulong flags;
	int data;
	unsigned long dev_offset = 0;
	unsigned long base_ofst = 0;

	base_ofst = HDMIRX_PHY_BASE_T3X + HDMIRX_PHY_OFFSET_T3X * port;
	data = rx_rd_reg_b(base_ofst + addr);
	return data;
}

void hdmirx_wr_amlphy_t3x(unsigned long addr, unsigned long data, unsigned char port)
{
	ulong flags;
	unsigned long base_ofst = 0;

	base_ofst = HDMIRX_PHY_BASE_T3X + HDMIRX_PHY_OFFSET_T3X * port;
	rx_wr_reg_b(base_ofst + addr, data);
}

void aml_phy_cfg_t3x_20(unsigned char port)
{
	unsigned long data32;

	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, 0x00f77666, port);
	udelay(5);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_DFE, 0x40100459, port);
	udelay(5);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, 0x04000095, port);
	udelay(5);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, 0x30880065, port);
	udelay(5);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, 0xffe00080, port);
	udelay(5);
	data32 = 0x11c73002;
	/* rterm en */
	data32 &= (~(0xf << 28));
	data32 |= (0xf << 28);
	/* port select */
	data32 &= (~(0xf << 24));
	data32 |= ((1 << port) << 24);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, data32, port);
}

void aml_phy_cfg_t3x_21(unsigned char port)
{
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0x1ff777f, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, 0x10f7000f, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC2, 0x00001a00, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, 0x0370ffff, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, 0x05ff1a05, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI, 0x51070030, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL, 0x07f06555, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, 0x04407cc2, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, 0x30133069, port);
	//printf("rx_21_phy_cfg\n");
}

void hdmirx_phy_init(void)
{
	aml_phy_cfg_t3x_20(0);
	aml_phy_cfg_t3x_20(1);
	aml_phy_cfg_t3x_21(2);
	aml_phy_cfg_t3x_21(3);
}

void top_common_init(void)
{
	unsigned long data32 = 0;

	hdmirx_wr_single_top(TOP_SW_RESET, 0, E_PORT0);
	hdmirx_wr_single_top(TOP_SW_RESET, 0, E_PORT1);
	hdmirx_wr_single_top(TOP_SW_RESET, 0, E_PORT2);
	hdmirx_wr_single_top(TOP_SW_RESET, 0, E_PORT3);
	hdmirx_wr_common_top(HDMIRX_TOP_SW_RESET_COMMON, 0);
	data32 = 0;
	/* bit4: hpd override, bit5: hpd reverse */
	data32 |= 1 << 4;
	data32 |= 1 << 5;
	data32 |= 0xf << 0;
	/* pull down all the hpd */
	hdmirx_wr_common_top(TOP_HPD_PWR5V, data32);
}

void hdmirx_init(void)
{
	top_common_init();
	hdmirx_phy_init();
	udelay(500);
	printf("rx 30-240620\n");
}

int hdmirx_get_squelch_sts(void)
{
	int sts_ch0, sts_ch1, sts_ch2, sts_ch3;
	int ret = 0;

	sts_ch0 = hdmirx_rd_single_top(TOP_MISC_STAT0_T3X, E_PORT0) & 1;
	sts_ch1 = hdmirx_rd_single_top(TOP_MISC_STAT0_T3X, E_PORT1) & 1;
	sts_ch2 = hdmirx_rd_single_top(TOP_MISC_STAT0_T3X, E_PORT2) & 1;
	sts_ch3 = hdmirx_rd_single_top(TOP_MISC_STAT0_T3X, E_PORT3) & 1;
	if (sts_ch0 || sts_ch1 || sts_ch2 || sts_ch3)
		ret = 1;
	return ret;
}
static int sqo_sts_init;
void vHDMIRX_task(void *pvParameters)
{
	unsigned long buf[4] = {0};
	unsigned long sqo_sts;

	hdmirx_init();
	sqo_sts_init = hdmirx_get_squelch_sts();
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(20));
		sqo_sts = hdmirx_get_squelch_sts();
		if (sqo_sts_init == 1) {
			if (sqo_sts)
				sqo_sts_init = 0;
		} else if (!sqo_sts_init) {
			if (!sqo_sts)
				sqo_sts_init = 2;
		} else if ((sqo_sts_init == 2)) {
			if (sqo_sts)
				sqo_sts_init = 3;
		}
		if (sqo_sts && sqo_sts_init == 3) {
			buf[0] = HDMI_RX_WAKEUP;
			STR_Wakeup_src_Queue_Send_FromISR(buf);
			printf("rx sqo_rise,wakeup\n");
			break;
		}
	}
	for ( ;; ) {
		vTaskDelay(pdMS_TO_TICKS(100));
		//printf("%s idle\n", __func__);
	}
}
