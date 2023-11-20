/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __STICK_MEM_H_
#define __STICK_MEM_H_

#define WATCHDOG_REBOOT		0xD

enum stick_mem_idx {
	STICK_WAKEUP_REASON = 0,
	STICK_VRTC = 1,
	STICK_REBOOT_FLAG = 2,
	STICK_FLAG_1 = 30,
	STICK_FLAG_2 = 31,
	STICK_MAX = 32
};

#define STICK_MEM_FLAG_1 0x01234567
#define STICK_MEM_FLAG_2 0xfedcba98
extern unsigned int *p_stick_mem;

int stick_mem_read(enum stick_mem_idx index, unsigned int *buf);
int stick_mem_write(enum stick_mem_idx index, unsigned int val);
void stick_mem_init(void);
unsigned int get_stick_reboot_flag(void);

#endif
