/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _GPIO_IRQ_H_
#define _GPIO_IRQ_H_

#include <gpio.h>

/**
 * enum GpioEEIRQLineType - type of gpio EE IRQ Lines
 */
enum GpioEEIRQLineType {
	GPIO_EE_IRQ_L0 = 0x0,
	GPIO_EE_IRQ_L1,
	GPIO_EE_IRQ_L2,
	GPIO_EE_IRQ_L3,
	GPIO_EE_IRQ_L4,
	GPIO_EE_IRQ_L5,
	GPIO_EE_IRQ_L6,
	GPIO_EE_IRQ_L7,
	GPIO_EE_IRQ_LINE_INVALID,
};

enum GPioAOIRQLineType {
	GPIO_AO_IRQ_L0 = 0x0,
	GPIO_AO_IRQ_L1,
	GPIO_AO_IRQ_LINE_INVALID,
};


typedef struct ParentIRQDesc {
	GpioIRQHandler_t phandler;
	uint32_t flags;
	uint16_t owner;
	uint16_t irq;
} ParentIRQDesc_t;

typedef struct GpioIRQBank {
	const char *name;
	ParentIRQDesc_t *parentIRQs;
	uint16_t gpioIRQBase;
	uint8_t parentIRQNum;
} GpioIRQBank_t;

#define LEN_NAME		32

#ifdef PADCTRL_GPIO_IRQ_CTRL0
#define GPIO_EE_IRQ_BASE	PADCTRL_GPIO_IRQ_CTRL0
#else
#define GPIO_EE_IRQ_BASE	(0xffd00000 + (0x3c20  << 2))
#endif

#define PARENT_IRQ_BK(p, f, o, n)					\
{									\
	.phandler = p,							\
	.flags    = f,							\
	.owner    = o,							\
	.irq      = n,							\
}

#define GPIO_IRQ_BK(n, b, i, in)					\
{									\
	.name		= n,						\
	.gpioIRQBase	= b,						\
	.parentIRQs	= i,						\
	.parentIRQNum	= in,						\
}

const GpioIRQBank_t *pGetGpioIrqBank(void);
void prvGpioPlatIrqSetup(uint16_t irqNum, uint8_t line, uint32_t flags);
#endif
