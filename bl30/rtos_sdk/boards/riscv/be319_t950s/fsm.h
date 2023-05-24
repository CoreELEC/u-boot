/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __FSM_H__
#define __FSM_H__

void vCoreFsmIdleInit(void);
extern void vRtcInit(void);
extern void create_str_task(void);
extern void trap_entry(void);
extern void irq_entry(void);

#endif
