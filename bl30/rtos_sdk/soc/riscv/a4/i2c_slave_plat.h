/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * platform related header file
 */
#ifndef _MESON_I2C_SLAVE_PLAT_H_
#define _MESON_I2C_SLAVE_PLAT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "FreeRTOS.h"
#include <common.h>
#include <meson_i2c_slave.h>

struct xMesonI2cSlavePlatdata i2cSlaveData[] = {
	{0, 0xfe08e400, 137, 0x20, 0xfe08e900, 4},
};

#ifdef __cplusplus
}
#endif
#endif /* _MESON_I2C_SLAVE_PLAT_H_ */
