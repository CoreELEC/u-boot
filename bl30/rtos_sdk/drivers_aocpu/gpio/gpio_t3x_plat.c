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

#define REG_PIN_SC2_SEL 0x04
#define REG_EDGE_POL_EXTR 0x1c
#define REG_EDGE_POL_MASK_SC2(x)                                                                   \
	({                                                                                         \
		typeof(x) _x = (x);                                                                \
		BIT(_x) | BIT(12 + (_x));                                                          \
	})
#define GPIO_IRQ_FILTER_SHIFT(x) (((x) % 2 == 0) ? 8 : 24)
#define GPIO_IRQ_POL_SHIFT(x) (BIT(0 + (x)))
#define GPIO_IRQ_EDGE_SHIFT(x) (BIT(12 + (x)))
#define GPIO_IRQ_BOTH_SHIFT(x) (BIT(0 + (x)))

static const struct GpioDomain eeDomain = {
	.name = "EE",
	.rPullen = PADCTRL_GPIOD_I,
	.rPull = PADCTRL_GPIOD_I,
	.rGpio = PADCTRL_GPIOD_I,
	.rMux = PADCTRL_PIN_MUX_REG0,
	.rDrv = PADCTRL_GPIOD_I,
};

static const struct GpioBank gpioBanks[BANK_NUM_MAX] = {
	/* name   pin_num   domain   pullen   pull   dir   out   in   mux   drv  */
	BANK_V2("D",  15, &eeDomain, 0x03, 0,  0x04, 0,  0x02, 0,  0x01, 0,  0x00, 0,
		0xb,  0,  0x07, 0),
	BANK_V2("E",   3, &eeDomain, 0x0b, 0,  0x0c, 0,  0x0a, 0,  0x09, 0,  0x08, 0,
		0xd,  0,  0x0f, 0),
	BANK_V2("Z",  16, &eeDomain, 0x13, 0,  0x14, 0,  0x12, 0,  0x11, 0,  0x10, 0,
		0x4,  0,  0x17, 0),
	BANK_V2("Z1",  4, &eeDomain, 0x13, 16, 0x14, 16, 0x12, 16, 0x11, 16, 0x10, 16,
		0x6,  0,  0xc0, 0),
	BANK_V2("H",  16, &eeDomain, 0x1b, 0,  0x1c, 0,  0x1a, 0,  0x19, 0,  0x18, 0,
		0x7,  0,  0x1f, 0),
	BANK_V2("H1", 14, &eeDomain, 0x1b, 16, 0x1c, 16, 0x1a, 16, 0x19, 16, 0x18, 16,
		0x9,  0,  0xc1, 0),
	BANK_V2("B",  14, &eeDomain, 0x2b, 0,  0x2c, 0,  0x2a, 0,  0x29, 0,  0x28, 0,
		0x00, 0,  0x2f, 0),
	BANK_V2("C",  11, &eeDomain, 0x33, 0,  0x34, 0,  0x32, 0,  0x31, 0,  0x30, 0,
		0x2,  0,  0x37, 0),
	BANK_V2("P",  10, &eeDomain, 0x3b, 0,  0x3c, 0,  0x3a, 0,  0x39, 0,  0x38, 0,
		0x14, 0,  0x3f, 0),
	BANK_V2("W",  16, &eeDomain, 0x63, 0,  0x64, 0,  0x62, 0,  0x61, 0,  0x60, 0,
		0x12, 0,  0x67, 0),
	BANK_V2("W1",  1, &eeDomain, 0x63, 16, 0x64, 16, 0x62, 16, 0x61, 16, 0x60, 16,
		0x11, 28, 0x67, 16),
	BANK_V2("M",  30, &eeDomain, 0x73, 0,  0x74, 0,  0x72, 0,  0x71, 0,  0x70, 0,
		0xe,  0,  0x77, 0),
	BANK_V2("TEST_N", 1, &eeDomain, 0x83, 0, 0x84, 0, 0x82, 0, 0x81, 0,  0x80, 0,
		0x15, 16, 0x87, 0),
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
	GPIO_IRQ_BK("D",  42, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("E",  57, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("Z",  59, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("Z1", 59, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("H",  109, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("H1", 109, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("B",  0,  eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("C",  14, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("P",  143, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("W",  25, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("W1", 41, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("M",  79, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("TEST_N", 139, eeIRQs, ARRAY_SIZE(eeIRQs)),
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
	reg_offset = REG_PIN_SC2_SEL + ((line / 2) << 2);

	/* clear both edge */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_EDGE_POL_EXTR, GPIO_IRQ_BOTH_SHIFT(line), 0);

	/* set filter */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + reg_offset, 0x7 << GPIO_IRQ_FILTER_SHIFT(line),
			  0x7 << GPIO_IRQ_FILTER_SHIFT(line));

	/* select trigger pin */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + reg_offset, 0x7f << bit_offset, irqNum << bit_offset);

	/* set trigger both type */
	if (flags & IRQF_TRIGGER_BOTH) {
		val |= GPIO_IRQ_BOTH_SHIFT(line);
		REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_EDGE_POL_EXTR, GPIO_IRQ_BOTH_SHIFT(line),
				  val);
		return;
	}

	/* set trigger single edge or level  type */
	if (flags & (IRQF_TRIGGER_LOW | IRQF_TRIGGER_FALLING))
		val |= GPIO_IRQ_POL_SHIFT(line);

	if (flags & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING))
		val |= GPIO_IRQ_EDGE_SHIFT(line);

	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE, REG_EDGE_POL_MASK_SC2(line), val);
}
