/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SOC_H
#define __SOC_H

#define SOC_PIC_NUM_INTERRUPTS 112
#define SOC_PIC_CTRL_ADDR 0x03000000
#define SOC_TMR_CTRL_ADDR 0x02000000

#if (SOC_PIC_NUM_INTERRUPTS / 32) * 32 == SOC_PIC_NUM_INTERRUPTS
#define IRQ_EN_REG_NUM (SOC_PIC_NUM_INTERRUPTS / 32)
#else
#define IRQ_EN_REG_NUM (SOC_PIC_NUM_INTERRUPTS / 32 + 1)
#endif

/*SoC/Shadow register mapping*/
#define VRTC_PARA_REG AO_DEBUG_REG2
#define VRTC_STICKY_REG AO_RTI_STICKY_REG2

#define TIMERE_LOW_REG ISA_TIMERE
#define TIMERE_HIG_REG ISA_TIMERE_HI
#define WAKEUP_REASON_STICK_REG PREG_STICKY_REG3
#define FSM_TRIGER_CTRL AO_TIMER_CTRL
#define FSM_TRIGER_SRC AO_DEBUG_REG2 //Just for run happy. Need fix it!!!!

#define UART_PORT_CONS UART1_WFIFO
#endif
