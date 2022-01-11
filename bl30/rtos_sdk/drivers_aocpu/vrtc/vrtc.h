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
#endif //__VRTC_H__
