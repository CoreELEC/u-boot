/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef INTERRUPT_CONTROL_H_
#define INTERRUPT_CONTROL_H_

#include "n200_pic_tmr.h"
#include "interrupt.h"
#include "gcc_compiler_attributes.h"

void pic_init(uintptr_t base_addr, uint32_t num_sources, uint32_t num_priorities);

void pic_set_threshold(uint32_t threshold);

void pic_enable_interrupt(uint32_t source);

void pic_disable_interrupt(uint32_t source);

void pic_set_priority(uint32_t source, uint32_t priority);

uint32_t pic_claim_interrupt(void);

void pic_complete_interrupt(uint32_t source);

uint32_t pic_check_eip(void);

void DefaultInterruptHandler(void);

// The interrupt 0 is empty
__weak function_ptr_t pic_interrupt_handlers[PIC_NUM_INTERRUPTS];

#endif
