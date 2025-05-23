/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __VRTC_H__
#define __VRTC_H__

void set_rtc(uint32_t val);
int get_rtc(uint32_t *val);
void vRTC_update(void);
void *xMboxSetRTC(void *msg);
void *xMboxGetRTC(void *msg);
void vRtcInit(void);
void alarm_set(void);
void alarm_clr(void);
void vCreat_alarm_timer(void);

struct rtc_time {
	int tm_sec;
	int tm_min;
};

extern uint32_t power_mode;

/* get time interface, time[4]:0-1:min, 2-3:sec */
void poweroff_get_rtc_min_sec(char *time);

#endif //__VRTC_H__
