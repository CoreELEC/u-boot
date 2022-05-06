/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef N200_TIMER_H
#define N200_TIMER_H

#include "soc.h"

#define TIMER_MSIP 0x0000
#define TIMER_MSIP_size 0x4
#define TIMER_MTIMECMP 0x0020
#define TIMER_MTIMECMP_size 0x8
#define TIMER_MTIME 0x0010
#define TIMER_MTIME_size 0x8

#define TIMER_CTRL_ADDR           SOC_TMR_CTRL_ADDR
#define TIMER_REG(offset)         _REG32(TMR_CTRL_ADDR, offset)
#define TIMER_FREQ                0

#endif
