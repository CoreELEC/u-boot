/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SOC_H
#define __SOC_H
#ifndef __ASM
#include "FreeRTOSConfig.h"
#include "riscv_const.h"
#include "irq.h"
#include "register.h"
#endif

#define SOC_ECLIC_NUM_INTERRUPTS 32
#define SOC_TIMER_FREQ           configCPU_CLOCK_HZ
#define SOC_ECLIC_CTRL_ADDR      0x0C000000UL
#define SOC_TIMER_CTRL_ADDR      0x02000000UL
#define SOC_PMP_BASE             0xff100000UL
#define SOC_LOCAL_SRAM_BASE      0x10000000UL
#define SRAM_BEGIN               SOC_LOCAL_SRAM_BASE
#define SRAM_SIZE                (0x20000)//(96*1024)
#define SRAM_END                 (SRAM_BEGIN + SRAM_SIZE)
#define IO_BASE                  0xff000000UL
#define IO_SIZE                  0x00100000
#define IO_BEGIN                 (IO_BASE)
#define IO_END                   (IO_BASE + IO_SIZE)

/*SoC/Shadow register mapping*/
#define VRTC_PARA_REG AO_DEBUG_REG2
#define VRTC_STICKY_REG AO_RTI_STICKY_REG2

#define TIMERE_LOW_REG ISA_TIMERE
#define TIMERE_HIG_REG ISA_TIMERE_HI
#define WAKEUP_REASON_STICK_REG PREG_STICKY_REG3
#define FSM_TRIGER_CTRL AO_TIMER_CTRL
#define FSM_TRIGER_SRC AO_DEBUG_REG2 //Just for run happy. Need fix it!!!!

#define UART_PORT_CONS AO_UART_WFIFO
#endif
