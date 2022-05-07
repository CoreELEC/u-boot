/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __RTC_H__
#define __RTC_H__

#define RTC_IRQ (131)
/* RTC enbale bit */
#define RTC_CTRL_EN (12)
/* RTC alarm enbale bit */
#define RTC_CTRL_ALM0_EN (0)
/* RTC INT irq status bit */
#define RTC_INT_IRQ (8)
/* RTC INT alarm0 irq status bit */
#define RTC_INT_ALM0_IRQ (0)
/* RTC INT clear alarm0 irq bit */
#define RTC_INT_CLR_ALM0_IRQ (0)

/* REBOOT_MODE */
#define COLD_REBOOT (0)

void *MboxGetRTC(void *msg);
void *MboxSetRTC(void *msg);
void rtc_init(void);
void store_rtc(void);
#endif //__RTC_H__
