/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "register.h"
#include "common.h"
#include "stdio.h"
#include "timer_source.h"

uint32_t timere_read(void)
{
	uint32_t time = 0;
	unsigned long long te = 0, temp = 0;

	/*timeE high+low, first read low, second read high*/
	te = REG32(TIMERE_LOW_REG);
	temp = REG32(TIMERE_HIG_REG);
	te += (temp << 32);
	te = te / 1000000;
	time = (uint32_t)te;
	//printf("----------time_e: %us\n", time);
	return time;
}

unsigned long long timere_read_us(void)
{
	unsigned long long te = 0, temp = 0;

	/*timeE high+low, first read low, second read high*/
	te = REG32(TIMERE_LOW_REG);
	temp = REG32(TIMERE_HIG_REG);
	te += (temp << 32);

	return te;
}

void udelay(uint32_t uS)
{
	unsigned long long t0;

	if (uS == 0)
		return;
	t0 = timere_read_us();
	while (timere_read_us() - t0 < uS)
		;
}

void mdelay(uint32_t mS)
{
	if (mS == 0)
		return;
	while (mS--)
		udelay(1000);
}
