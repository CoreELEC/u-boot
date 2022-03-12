/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */


/*
 * vad suspend header file
 */

#ifndef _VAD_SUSPEND_H_
#define _VAD_SUSPEND_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VAD_TASK_PRI                      5
#define DSP_VAD_WAKUP_ARM      0x5555AAAA
#define WAIT_SWITCH_TO_24MHZ   0x5A5A5A5A
#define WAIT_SWITCH_TO_RTC_PLL 0xA5A5A5A5
#define WAKEUP_FROM_OTHER_KEY  0xA8A8A8A8

#ifdef __cplusplus
}
#endif
#endif
