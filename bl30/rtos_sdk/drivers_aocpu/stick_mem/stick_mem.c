/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "stick_mem.h"
#include "FreeRTOSConfig.h"

extern unsigned int __stick_base;
static unsigned int last_stick_reboot_flag;
unsigned int *p_stick_mem = (unsigned int *)configSTICK_MEM_ADDR; //&__stick_base;

int stick_mem_read(enum stick_mem_idx index, unsigned int *buf)
{
	if ((p_stick_mem[STICK_FLAG_1] != STICK_MEM_FLAG_1) ||
	    (p_stick_mem[STICK_FLAG_2] != STICK_MEM_FLAG_2)) {
		/*cprintf(CC_SYSTEM, "read stick mem bad value!\n");*/
		return -1;
	}

	if (index >= STICK_MAX) {
		printf("read stick mem bad index=%d!\n", index);
		return -2;
	}

	*(buf) = p_stick_mem[index];

	return 0;
}

int stick_mem_write(enum stick_mem_idx index, unsigned int val)
{
	if (index >= STICK_MAX) {
		printf("write stick mem bad index=%d!\n", index);
		return -2;
	}

	if (p_stick_mem[STICK_FLAG_1] != STICK_MEM_FLAG_1)
		p_stick_mem[STICK_FLAG_1] = STICK_MEM_FLAG_1;

	if (p_stick_mem[STICK_FLAG_2] != STICK_MEM_FLAG_2)
		p_stick_mem[STICK_FLAG_2] = STICK_MEM_FLAG_2;

	p_stick_mem[index] = val;

	return 0;
}

void stick_mem_init(void)
{
	unsigned int i;

	/* this is warm boot, get last stick reboot flag */
	if ((p_stick_mem[STICK_FLAG_1] == STICK_MEM_FLAG_1) &&
	    (p_stick_mem[STICK_FLAG_2] == STICK_MEM_FLAG_2))
		last_stick_reboot_flag = p_stick_mem[STICK_REBOOT_FLAG];
	/* this is cold boot, so clear stick memory for init */
	else {
		for (i = 0; i < STICK_MAX; i++)
			p_stick_mem[i] = 0;
	}
}

unsigned int get_stick_reboot_flag(void)
{
	return last_stick_reboot_flag;
}
