/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "common.h"
#include "vrtc.h"
#include "timer_source.h"
#include "register.h"
#include "FreeRTOS.h"
#include "mailbox-api.h"
#include "timers.h"
#include "suspend.h"
#include "soc.h"
#include "string.h"

#undef TAG
#define TAG "VRTC"
/* Timer handle */
//TimerHandle_t xRTCTimer = NULL;
static uint32_t last_time;

void set_rtc(uint32_t val)
{
	REG32(VRTC_STICKY_REG) = val;
	/*The last time update RTC*/
	last_time = timere_read();
}

int get_rtc(uint32_t *val)
{
	if (!REG32(VRTC_STICKY_REG))
		return -1;

	*(val) = REG32(VRTC_STICKY_REG);
	return 0;
}

void vRTC_update(void)
{
	uint32_t val;

	if (!get_rtc(&val)) {
		val += timere_read() - last_time;
		set_rtc(val);
	}
}

void *xMboxSetRTC(void *msg)
{
	unsigned int val = *(uint32_t *)msg;

	printf("[%s]: %s val=0x%x\n", TAG, __func__, val);
	set_rtc(val);

	return NULL;
}

void *xMboxGetRTC(void *msg)
{
	uint32_t val = 0;

	get_rtc(&val);
	memset(msg, 0, MBOX_BUF_LEN);
	*(uint32_t *)msg = val;
	printf("[%s]: %s val=0x%x\n", TAG, __func__, val);

	return NULL;
}

void vRtcInit(void)
{
	int ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_SET_RTC, xMboxSetRTC, 0);
	if (ret == MBOX_CALL_MAX)
		printf("[%s]: mbox cmd 0x%x register fail\n", TAG, MBX_CMD_SET_RTC);

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_GET_RTC, xMboxGetRTC, 1);
	if (ret == MBOX_CALL_MAX)
		printf("[%s]: mbox cmd 0x%x register fail\n", TAG, MBX_CMD_GET_RTC);
}

static TimerHandle_t xRTCTimer;
static uint32_t time_start;

void alarm_set(void)
{
	uint32_t val;

	val = REG32(VRTC_PARA_REG);

	if (val) {
		printf("[%s]: alarm val=%d S\n", TAG, val);
		time_start = timere_read();
		if (xRTCTimer)
			xTimerStart(xRTCTimer, 0);
	}
}

void alarm_clr(void)
{
	time_start = 0;
	xTimerStop(xRTCTimer, 0);
}

static void valarm_update(TimerHandle_t xTimer)
{
	uint32_t val;
	uint32_t buf[4] = { 0 };

	(void)xTimer;
	val = REG32(VRTC_PARA_REG);

	if (time_start && (timere_read() - time_start > val)) {
		buf[0] = RTC_WAKEUP;

		printf("[%s]: vrtc alarm fired\n", TAG);
		REG32(VRTC_PARA_REG) = 0;
		STR_Wakeup_src_Queue_Send(buf);
	}
}

void vCreat_alarm_timer(void)
{
	xRTCTimer = xTimerCreate("Timer", pdMS_TO_TICKS(1000), pdTRUE, NULL, valarm_update);
}
