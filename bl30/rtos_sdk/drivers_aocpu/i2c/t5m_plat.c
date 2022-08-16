/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include <common.h>
#include "util.h"
#include "meson_i2c.h"

#define HHI_GCLK_MPEG0 ((0x050 << 2) + 0xff63c000)

struct xMesonI2cPlatdata t5w_i2c_data[] = {
	{ 0, 0xffd1f000, 3, 15, 100000, MESON_I2C_CLK_RATE, HHI_GCLK_MPEG0, 9 }, /* i2c A */
	{ 1, 0xffd1e000, 3, 15, 100000, MESON_I2C_CLK_RATE, HHI_GCLK_MPEG0, 9 }, /* i2c B */
	{ 2, 0xffd1d000, 3, 15, 100000, MESON_I2C_CLK_RATE, HHI_GCLK_MPEG0, 9 }, /* i2c C */
	{ 3, 0xffd1c000, 3, 15, 100000, MESON_I2C_CLK_RATE, HHI_GCLK_MPEG0, 9 }, /* i2c D */
};

void meson_i2c_plat_init(void)
{
	plat = t5w_i2c_data;
}
