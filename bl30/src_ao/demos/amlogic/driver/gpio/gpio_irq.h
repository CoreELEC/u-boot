/*
 * Copyright (C) 2014-2018 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * gpio IRQ header file
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

#define GPIO_EE_IRQ_BASE  PADCTRL_GPIO_IRQ_CTRL0

#define REG_PIN_SC2_SEL			0x04
#define REG_EDGE_POL_EXTR		0x1c
#define REG_EDGE_POL_MASK_SC2(x)			\
	({typeof(x) _x = (x); BIT(_x) | BIT(12 + (_x)); })
#define GPIO_IRQ_FILTER_SHIFT(x)	(((x) % 2 == 0) ? 8 : 24)

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
#endif
