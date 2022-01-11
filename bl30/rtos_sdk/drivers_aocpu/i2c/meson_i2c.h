/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MESON_I2C_H
#define __MESON_I2C_H

#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"

	enum {
		I2C_M0 = 0,
		I2C_M1,
		I2C_M2,
		I2C_M3,
		I2C_M4,
		I2C_M5,
		I2C_AO_A,
		I2C_AO_B
	};


	struct xI2cMsg {
		uint32_t addr;
		uint32_t flags;
		uint32_t len;
		uint8_t *buf;
	};

	struct xMesonI2cPlatdata {
		uint32_t bus_num;
		unsigned long reg;
		uint32_t div_factor;
		uint32_t delay_ajust;
		uint32_t clock_frequency;	/* i2c rate */
		uint32_t clkin_rate;
		uint32_t clk_base;
		uint32_t clk_offset;
	};

#define I2C_M_RD		0x0001
#define MESON_I2C_CLK_RATE      166666667

	void meson_i2c_plat_init(void);
	int32_t xI2cMesonPortInit(uint32_t id);
	int32_t xI2cMesonSetBusSpeed(uint32_t speed);
	int32_t xI2cMesonXfer(struct xI2cMsg *msg, uint32_t nmsgs);
	int32_t xI2cMesonRead(uint32_t addr, uint8_t offset,
			      uint8_t *buffer, uint32_t len);
	int32_t xI2cMesonWrite(uint32_t addr, uint8_t offset,
			       uint8_t *buffer, uint32_t len);
	void xI2cDumpRegs(void);
	void xI2cSetCurrentId(uint32_t id);

	extern struct xMesonI2cPlatdata *plat;
#ifdef __cplusplus
}
#endif
#endif
