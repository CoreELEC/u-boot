/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __POWER_H__
#define __POWER_H__

extern void str_hw_init(void);
extern void str_hw_disable(void);
extern void str_power_on(int shutdown_flag);
extern void str_power_off(int shutdown_flag);
void Bt_GpioIRQRegister(void);
void Bt_GpioIRQFree(void);

#endif
