/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "rtc.h"
#include "register.h"
#include "FreeRTOS.h"
#include "mailbox-api.h"
#include "soc.h"
#include "string.h"
#include "interrupt.h"
#include "suspend.h"
#include "rtc_register.h"

#undef TAG
#define TAG "AOCPU RTC"

static uint32_t gray_to_binary(uint32_t gray)
{
	uint32_t bcd = gray;
	int size = sizeof(bcd) * 8;
	int i;

	for (i = 0; (1 << i) < size; i++)
		bcd ^= bcd >> (1 << i);

	return bcd;
}

static void vRTCInterruptHandler(void)
{
	uint32_t buf[4] = { 0 };

	printf("[%s]: rtc alarm fired\n", TAG);

	buf[0] = RTC_WAKEUP;
	STR_Wakeup_src_Queue_Send_FromISR(buf);
}

static uint32_t get_reboot_mode(void)
{
	uint32_t reg_val;
	uint32_t reboot_mode;

	reg_val = REG32(SYSCTRL_SEC_STATUS_REG31);
	reboot_mode = ((reg_val >> 12) & 0xf);

	return reboot_mode;
}

static void reset_rtc(void)
{
	uint32_t reg_val;

	printf("[%s]: reset rtc\n", TAG);
	/* Reset RTC */
	reg_val = (1 << 0);
	REG32(RESETCTRL_RESET4) = reg_val;
	/* Mask RTC reset to prevent RTC being reset in the next reboot */
	reg_val = REG32(RESETCTRL_RESET4_MASK);
	reg_val |= (1 << 0);
	REG32(RESETCTRL_RESET4_MASK) = reg_val;
}

static int get_rtc(uint32_t *val)
{
	if (!REG32(VRTC_STICKY_REG))
		return -1;

	*(val) = REG32(VRTC_STICKY_REG);
	return 0;
}

static void set_rtc(uint32_t val)
{
	REG32(VRTC_STICKY_REG) = val;
}

void store_rtc(void)
{
	uint32_t reg_val;

	reg_val = REG32(RTC_DIG_REAL_TIME);
#ifdef CONFIG_RTC_STORAGE_FORMAT_GRAY
	reg_val = gray_to_binary(reg_val);
#endif
	REG32(VRTC_STICKY_REG) = reg_val;
}

void *MboxSetRTC(void *msg)
{
	unsigned int val = *(uint32_t *)msg;

	printf("[%s]: %s val=0x%x\n", TAG, __func__, val);
	set_rtc(val);

	return NULL;
}

void *MboxGetRTC(void *msg)
{
	uint32_t val = 0;

	get_rtc(&val);
	memset(msg, 0, MBOX_BUF_LEN);
	*(uint32_t *)msg = val;

	printf("[%s]: %s val=0x%x\n", TAG, __func__, val);

	return NULL;
}

void rtc_enable_irq(void)
{
	int ret;

	ret = RegisterIrq(RTC_IRQ, 6, vRTCInterruptHandler);
	if (ret)
		printf("RTC_irq RegisterIrq error, ret = %d\n", ret);
	EnableIrq(RTC_IRQ);
}

void rtc_disable_irq(void)
{
	int ret;

	DisableIrq(RTC_IRQ);
	ret = UnRegisterIrq(RTC_IRQ);
	if (ret)
		printf("RTC_irq UnRegisterIrq error, ret = %d\n", ret);
}

void rtc_init(void)
{
	int ret;
	uint32_t reboot_mode;

	printf("[%s]: init rtc\n", TAG);

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_SET_RTC, MboxSetRTC, 0);
	if (ret == MBOX_CALL_MAX)
		printf("[%s]: mbox cmd 0x%x register fail\n", TAG, MBX_CMD_SET_RTC);

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_GET_RTC, MboxGetRTC, 1);
	if (ret == MBOX_CALL_MAX)
		printf("[%s]: mbox cmd 0x%x register fail\n", TAG, MBX_CMD_GET_RTC);

	reboot_mode = get_reboot_mode();
	if (reboot_mode == COLD_REBOOT)
		reset_rtc();
}
