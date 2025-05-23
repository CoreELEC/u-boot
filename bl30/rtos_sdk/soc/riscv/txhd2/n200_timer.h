/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef N200_TIMER_H
#define N200_TIMER_H

#include "soc.h"

#define TIMER_MSTOP 0xFF8
#define TIMER_MSIP 0xFFC
#define TIMER_MSIP_size 0x4
#define TIMER_MTIMECMP 0x8
#define TIMER_MTIMECMP_size 0x8
#define TIMER_MTIME 0x0
#define TIMER_MTIME_size 0x8

#define TIMER_CTRL_ADDR SOC_TIMER_CTRL_ADDR
#define TIMER_REG(offset) _REG32(TIMER_CTRL_ADDR, offset)
#define TIMER_FREQ SOC_TIMER_FREQ

#endif
