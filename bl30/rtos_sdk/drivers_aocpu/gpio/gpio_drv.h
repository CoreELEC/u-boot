/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _GPIO_DRIVER_H_
#define _GPIO_DRIVER_H_

/**
 * enum GpioRegType - type of registers encoded in @meson_reg_desc
 */
enum GpioRegType {
	REG_PULLEN = 0x0,
	REG_PULL,
	REG_DIR,
	REG_OUT,
	REG_IN,
	REG_MUX,
	REG_DRV,
	NUM_REG,
};

struct GpioRegDesc {
	uint8_t reg;
	uint8_t bit;
};

struct GpioDomain {
	const char *name;

	volatile uint32_t rPullen;
	volatile uint32_t rPull;
	volatile uint32_t rGpio;
	volatile uint32_t rMux;
	volatile uint32_t rDrv;
};

struct GpioBank {
	const char *name;
	const struct GpioDomain *domain;
	struct GpioRegDesc regs[NUM_REG];
};

const struct GpioBank *pGetGpioBank(void);

#define BANK(n, d, per, peb, pr, pb, dr, db, or, ob, ir, ib, mr, mb, sr, sb)                       \
	{                                                                                          \
		.name = n, .domain = d,                                                            \
		.regs = {                                                                          \
			[REG_PULLEN] = { per, peb }, [REG_PULL] = { pr, pb },                      \
			[REG_DIR] = { dr, db },	     [REG_OUT] = { or, ob },                       \
			[REG_IN] = { ir, ib },	     [REG_MUX] = { mr, mb },                       \
			[REG_DRV] = { sr, sb },                                                    \
		},                                                                                 \
	}

#endif
