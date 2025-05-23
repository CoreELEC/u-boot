/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __EXCEPTION_HANDLER_H__
#define __EXCEPTION_HANDLER_H__

#include <stdint.h>
#include "common.h"
#include "riscv_encoding.h"

#define RV_PIC_INT_SOFT 3
#define RV_PIC_INT_TIMER 7
#define RV_PIC_INT_FIQ 19
#define RV_PIC_INT_IRQ 20
#define RV_PIC_INT_FMAC 21
#define RV_PIC_INT_TSHI 22

#define RV_PIC_INT_IMAC 27
#define RV_PIC_INT_H2FW 28
#define RV_PIC_INT_COXT 29
#define RV_PIC_INT_WAKE 30
#define RV_PIC_INT_HIQ0 31
#define RV_PIC_INT_HIQ1 32
#define RV_PIC_INT_TSIQ 33
#define RV_PIC_INT_WDIQ 34
#define RV_PIC_INT_U2D 35
#define RV_PIC_INT_USB_IDDIG 36
#define RV_PIC_INT_USB_VBUSDIG 37

#define RV_PIC_LVL_WDIQ 7
#define RV_PIC_LVL_FIQ 7
#define RV_PIC_LVL_IRQ 5

#define RV_PIC_LVL_SOFT 7
#define RV_PIC_LVL_TIMR 7
#define RV_PIC_LVL_FMAC 6
#define RV_PIC_LVL_TSHI 6

#define RV_PIC_LVL_IMAC 5
#define RV_PIC_LVL_H2FW 5
#define RV_PIC_LVL_COXT 5
#define RV_PIC_LVL_WAKE 4

#define RV_PIC_LVL_USB_VBUSDIG 3
#define RV_PIC_LVL_USB_IDDIG 3
#define RV_PIC_LVL_U2D 3

#define RV_PIC_LVL_HIQ0 3
#define RV_PIC_LVL_HIQ1 2
#define RV_PIC_LVL_TSIQ 1

uint32_t interrupt_register_exception(uint32_t mcause, uint32_t sp);

uint32_t interrupt_register_nmi(uint32_t mcause, uint32_t sp);

#endif

