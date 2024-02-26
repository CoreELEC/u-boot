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

static void rtc_sec_to_tm(uint32_t sec, struct rtc_time *tm)
{
	uint32_t min;

	tm->tm_sec = sec % 60;
	min = sec / 60;
	tm->tm_min = min % 60;
}

void poweroff_get_rtc_min_sec(char *time)
{
	uint32_t secs;
	struct rtc_time tm;

	if (power_mode == 0xf) {
		secs = REG32(VRTC_STICKY_REG) + timere_read() - last_time;
		rtc_sec_to_tm(secs, &tm);

		sprintf(time, "%02d", tm.tm_min);
		sprintf(time+2, "%02d", tm.tm_sec);
	} else {
		printf("non-poweroff mode callback, invalid called !\n");
		time = NULL;
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
	if (ret)
		printf("[%s]: mbox cmd 0x%x register fail\n", TAG, MBX_CMD_SET_RTC);

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_GET_RTC, xMboxGetRTC, 1);
	if (ret)
		printf("[%s]: mbox cmd 0x%x register fail\n", TAG, MBX_CMD_GET_RTC);
}

#ifndef configVRTC_DISABLE_ALARM
static TimerHandle_t xRTCTimer;

void alarm_set(void)
{
	uint32_t val;

	val = REG32(VRTC_PARA_REG);

	if (val) {
		printf("[%s]: alarm val=%d S\n", TAG, val);
		if (xRTCTimer)
			xTimerChangePeriod(xRTCTimer, pdMS_TO_TICKS(val * 1000), 0);
	}
}

void alarm_clr(void)
{
	if (xRTCTimer)
		xTimerStop(xRTCTimer, 0);
}

static void valarm_update(TimerHandle_t xTimer)
{
	uint32_t buf[4] = { 0 };

	(void)xTimer;

	buf[0] = RTC_WAKEUP;
	printf("[%s]: vrtc alarm fired\n", TAG);
	REG32(VRTC_PARA_REG) = 0;
	STR_Wakeup_src_Queue_Send(buf);
}

void vCreat_alarm_timer(void)
{
	xRTCTimer = xTimerCreate("Timer", pdMS_TO_TICKS(1000), pdFALSE, NULL, valarm_update);
}
#endif
