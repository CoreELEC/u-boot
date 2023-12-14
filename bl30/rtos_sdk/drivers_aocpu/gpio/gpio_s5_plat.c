/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * gpio driver platform data
 */
#include "FreeRTOS.h"
#include <register.h>
#include <common.h>
#include <gpio.h>
#include "projdefs.h"
#include "gpio_drv.h"
#include "gpio_irq.h"
#include "portmacro.h"

/* gpio irq controller */
#define IRQ_GPIO0_NUM 288
#define IRQ_GPIO1_NUM 289
#define IRQ_GPIO2_NUM 290
#define IRQ_GPIO3_NUM 291
#define IRQ_GPIO4_NUM 292
#define IRQ_GPIO5_NUM 293
#define IRQ_GPIO6_NUM 294
#define IRQ_GPIO7_NUM 295

#define REG_PIN_P1_SEL			0x10
#define REG_EDGE_SINGLE			0x04
#define REG_POL_LOW			0x08
#define REG_EDGE_BOTH			0x0c
#define GPIO_IRQ_FILTER_SHIFT(x)	(((x) % 2 == 0) ? 8 : 24)
#define GPIO_IRQ_POL_SHIFT(x)		(BIT(0 + (x)))
#define GPIO_IRQ_EDGE_SHIFT(x)		(BIT(0 + (x)))
#define GPIO_IRQ_BOTH_SHIFT(x)		(BIT(0 + (x)))

static const struct GpioDomain stDomain = {
	.name = "ST",
	.rPullen = PADCTRL_GPIOB_I,
	.rPull = PADCTRL_GPIOB_I,
	.rGpio = PADCTRL_GPIOB_I,
	.rMux = PADCTRL_PIN_MUX_REG10,
	.rDrv = PADCTRL_GPIOB_I,
};

static const struct GpioDomain eeDomain = {
	.name = "EE",
	.rPullen = PADCTRL_TESTN_I,
	.rPull = PADCTRL_TESTN_I,
	.rGpio = PADCTRL_TESTN_I,
	.rMux = PADCTRL_PIN_MUX_REG0,
	.rDrv = PADCTRL_TESTN_I,
};

static const struct GpioBank gpioBanks[BANK_NUM_MAX] = {
	/* name   pin_num   domain   pullen   pull   dir   out   in   mux   drv  */
	BANK_V2("A", 11, &eeDomain, 0x13, 0, 0x14, 0, 0x12, 0, 0x11, 0, 0x10, 0,
		0x00, 0, 0x17, 0),
	BANK_V2("C",  8, &eeDomain, 0x23, 0, 0x24, 0, 0x22, 0, 0x21, 0, 0x20, 0,
		0x03, 0, 0x27, 0),
	BANK_V2("D", 12, &eeDomain, 0x33, 0, 0x34, 0, 0x32, 0, 0x31, 0, 0x30, 0,
		0x05, 0, 0x37, 0),
	BANK_V2("E",  5, &eeDomain, 0x43, 0, 0x44, 0, 0x42, 0, 0x41, 0, 0x40, 0,
		0x07, 0, 0x47, 0),
	BANK_V2("H",  9, &eeDomain, 0x53, 0, 0x54, 0, 0x52, 0, 0x51, 0, 0x50, 0,
		0x08, 0, 0x57, 0),
	BANK_V2("B", 13, &stDomain, 0x03, 0, 0x04, 0, 0x02, 0, 0x01, 0, 0x00, 0,
		0x00, 0, 0x07, 0),
	BANK_V2("T", 25, &eeDomain, 0x73, 0, 0x74, 0, 0x72, 0, 0x71, 0, 0x70, 0,
		0x0a, 0, 0x77, 0),
	BANK_V2("X", 20, &eeDomain, 0x83, 0, 0x84, 0, 0x82, 0, 0x81, 0, 0x80, 0,
		0x0e, 0, 0x87, 0),
	BANK_V2("Z", 16, &eeDomain, 0x93, 0, 0x94, 0, 0x92, 0, 0x91, 0, 0x90, 0,
		0x11, 0, 0x97, 0),
	BANK_V2("TEST_N", 1, &eeDomain, 0x03, 0, 0x04, 0, 0x02, 0, 0x01, 0, 0x00, 0,
		0x0a, 0, 0x07, 0),
};

static struct ParentIRQDesc eeIRQs[] = {
	[GPIO_EE_IRQ_L0] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_GPIO0_NUM),
	[GPIO_EE_IRQ_L1] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_GPIO1_NUM),
	[GPIO_EE_IRQ_L2] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_GPIO2_NUM),
	[GPIO_EE_IRQ_L3] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_GPIO3_NUM),
	[GPIO_EE_IRQ_L4] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_GPIO4_NUM),
	[GPIO_EE_IRQ_L5] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_GPIO5_NUM),
	[GPIO_EE_IRQ_L6] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_GPIO6_NUM),
	[GPIO_EE_IRQ_L7] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_GPIO7_NUM),
};

static const struct GpioIRQBank irqBanks[BANK_NUM_MAX] = {
	GPIO_IRQ_BK("A", 13,  eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("C", 24,  eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("D", 32,  eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("E", 44,  eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("H", 49,  eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("B", 0,   eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("T", 58,  eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("X", 83,  eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("Z", 103, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("TEST_N", 119, eeIRQs, ARRAY_SIZE(eeIRQs)),
};

const struct GpioBank *pGetGpioBank(void)
{
	return gpioBanks;
}

const struct GpioIRQBank *pGetGpioIrqBank(void)
{
	return irqBanks;
}

void prvGpioPlatIrqSetup(uint16_t irqNum, uint8_t line, uint32_t flags)
{
	uint32_t val = 0;
	uint32_t reg_offset = 0;
	uint16_t bit_offset = 0;

	bit_offset = ((line % 2) == 0) ? 0 : 16;
	reg_offset = REG_PIN_P1_SEL + ((line / 2) << 2);

	/* clear both edge */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_EDGE_BOTH,
			  GPIO_IRQ_BOTH_SHIFT(line), 0);
	/* set filter */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + reg_offset,
			  0x7 << GPIO_IRQ_FILTER_SHIFT(line),
			  0x7 << GPIO_IRQ_FILTER_SHIFT(line));

	/* select trigger pin */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + reg_offset,
			  0xff << bit_offset, irqNum << bit_offset);

	/* set trigger both type */
	if (flags & IRQF_TRIGGER_BOTH) {
		val |= GPIO_IRQ_BOTH_SHIFT(line);
		REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_EDGE_BOTH,
				  GPIO_IRQ_BOTH_SHIFT(line), val);
		return;
	}

	/* set trigger single edge or level  type */
	if (flags & (IRQF_TRIGGER_LOW | IRQF_TRIGGER_FALLING)) {
		val |= GPIO_IRQ_POL_SHIFT(line);
		REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_POL_LOW, GPIO_IRQ_POL_SHIFT(line), val);
	}

	if (flags & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)) {
		val = 0;
		val |= GPIO_IRQ_EDGE_SHIFT(line);
		REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_EDGE_SINGLE,
				  GPIO_IRQ_EDGE_SHIFT(line), val);
	}
}
