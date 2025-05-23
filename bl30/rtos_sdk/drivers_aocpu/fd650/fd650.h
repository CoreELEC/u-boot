/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __FD650H__
#define __FD650H__

#include "common.h"
#include "uart.h"
#include "FreeRTOS.h"

#define FD650_BIT_ENABLE	0x01
#define FD650_BIT_SLEEP		0x04
//#define FD650_BIT_7SEG	0x08
#define FD650_BIT_INTENS1	0x10
#define FD650_BIT_INTENS2	0x20
#define FD650_BIT_INTENS3	0x30
#define FD650_BIT_INTENS4	0x40
#define FD650_BIT_INTENS5	0x50
#define FD650_BIT_INTENS6	0x60
#define FD650_BIT_INTENS7	0x70
#define FD650_BIT_INTENS8	0x00

#define FD650_SYSOFF	0x0400
#define FD650_SYSON		(FD650_SYSOFF | FD650_BIT_ENABLE)
#define FD650_SLEEPOFF	FD650_SYSOFF
#define FD650_SLEEPON	(FD650_SYSOFF | FD650_BIT_SLEEP)
//#define FD650_7SEG_ON	(FD650_SYSON | FD650_BIT_7SEG)
#define FD650_8SEG_ON	(FD650_SYSON | 0x00)
#define FD650_SYSON_1	(FD650_SYSON | FD650_BIT_INTENS1)
#define FD650_SYSON_4	(FD650_SYSON | FD650_BIT_INTENS4)
#define FD650_SYSON_8	(FD650_SYSON | FD650_BIT_INTENS8)


#define FD650_DIG0		0x1400
#define FD650_DIG1		0x1500
#define FD650_DIG2		0x1600
#define FD650_DIG3		0x1700

#define FD650_DOT			0x0080

#define FD650_GET_KEY	0x0700

struct fd650_bus {
	/*
	 * udelay - delay [us] between GPIO toggle operations,
	 * which is 1/4 of I2C speed clock period.
	 */
	int bus_id;
	int udelay;
	 /* sda, scl */
	uint32_t sda;
	uint32_t scl;
};

enum FD650State {
	FD650_STATE_DEFAULT = 0,
	FD650_STATE_SHOW,
	FD650_STATE_TIME,
	FD650_STATE_INVALID,
};

uint8_t fd650_read(struct fd650_bus *bus);
void fd650_write(struct fd650_bus *bus, uint32_t cmd);
int fd650_bus_init(uint32_t id);
void led_show_650(char *str, unsigned char sec_flag, unsigned char lock);

#endif
