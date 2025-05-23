/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MESON_I2C_SLAVE_H
#define __MESON_I2C_SLAVE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "common.h"

extern uint32_t current_slave_id;
struct xMesonI2cSlavePlatdata {
	uint32_t index;
	unsigned long reg;
	uint32_t irq;
	uint32_t slave_addr;
	unsigned long reset_reg;
	uint32_t reset_bit;
};
/*for wake up, normal must be 0*/
int32_t xI2cSlaveMesonInit(uint32_t id, int normal);

#ifdef __cplusplus
}
#endif
#endif
