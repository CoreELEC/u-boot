/*
 * Copyright (c) 2021-2024 Amlogic, Inc. All rights reserved.
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
#define IRQ_GPIO0_NUM			10
#define IRQ_GPIO1_NUM			11
#define IRQ_GPIO2_NUM			12
#define IRQ_GPIO3_NUM			13
#define IRQ_GPIO4_NUM			14
#define IRQ_GPIO5_NUM			15
#define IRQ_GPIO6_NUM			16
#define IRQ_GPIO7_NUM			17

#define REG_PIN_SC2_SEL			0x04
#define REG_EDGE_POL_EXTR		0x1c
#define REG_EDGE_POL_MASK_SC2(x)			\
	({typeof(x) _x = (x); BIT(_x) | BIT(12 + (_x)); })
#define GPIO_IRQ_FILTER_SHIFT(x)	(((x) % 2 == 0) ? 8 : 24)
#define GPIO_IRQ_POL_SHIFT(x)		(BIT(0 + (x)))
#define GPIO_IRQ_EDGE_SHIFT(x)		(BIT(12 + (x)))
#define GPIO_IRQ_BOTH_SHIFT(x)		(BIT(0 + (x)))

static const struct GpioDomain eeDomain = {
	.name = "EE",
	.rPullen = PADCTRL_GPIOD_I,
	.rPull = PADCTRL_GPIOD_I,
	.rGpio = PADCTRL_GPIOD_I,
	.rMux = PADCTRL_PIN_MUX_REG0,
	.rDrv = PADCTRL_GPIOD_I,
};

static const struct GpioBank gpioBanks[BANK_NUM_MAX] = {
	/*   name   domain   pullen   pull   dir   out   in   mux   ds  */
	BANK("W",  &eeDomain, 0x063,  0, 0x064,  0, 0x062,  0, 0x061,  0, 0x060,  0,
	     0x012,  0,  0x067,  0),
	BANK("D",  &eeDomain, 0x003,  0, 0x004,  0, 0x002,  0, 0x001,  0, 0x000,  0,
	     0x00b,  0,  0x007,  0),
	BANK("E",  &eeDomain, 0x00b,  0, 0x00c,  0, 0x00a,  0, 0x009,  0, 0x008,  0,
	     0x00d,  0,  0x00f,  0),
	BANK("B",  &eeDomain, 0x02b,  0, 0x02c,  0, 0x02a,  0, 0x029,  0, 0x028,  0,
	     0x000,  0,  0x02f,  0),
	BANK("C",  &eeDomain, 0x033,  0, 0x034,  0, 0x032,  0, 0x031,  0, 0x030,  0,
	     0x002,  0,  0x037,  0),
	BANK("Z",  &eeDomain, 0x013,  0, 0x014,  0, 0x012,  0, 0x011,  0, 0x010,  0,
	     0x004,  0,  0x017,  0),
	BANK("Z1", &eeDomain, 0x013, 16, 0x014, 16, 0x012, 16, 0x011, 16, 0x010, 16,
	     0x006,  0,  0x0c0,  0),
	BANK("H",  &eeDomain, 0x01b,  0, 0x01c,  0, 0x01a,  0, 0x019,  0, 0x018,  0,
	     0x007,  0,  0x01f,  0),
	BANK("H1", &eeDomain, 0x01b, 16, 0x01c, 16, 0x01a, 16, 0x019, 16, 0x018, 16,
	     0x009,  0,  0x0c1,  0),
	BANK("M",  &eeDomain, 0x073,  0, 0x074,  0, 0x072,  0, 0x071,  0, 0x070,  0,
	     0x00e,  0,  0x077,  0),
	BANK("M1", &eeDomain, 0x073, 16, 0x074, 16, 0x072, 16, 0x071, 16, 0x070, 16,
	     0x010,  0,  0x0c2,  0),
	BANK("TEST_N", &eeDomain, 0x083, 0, 0x084, 0, 0x082, 0, 0x081, 0, 0x080,  0,
	     0x014,  0,  0x087,  0),
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
	GPIO_IRQ_BK("W",       25, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("D",       38, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("E",       53, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("B",        0, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("C",       14, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("Z",       56, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("Z1",      72, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("H",      106, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("H1",     122, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("M",       76, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("M1",      92, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("TEST_N", 128, eeIRQs, ARRAY_SIZE(eeIRQs)),
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
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_EDGE_POL_EXTR,
			  GPIO_IRQ_BOTH_SHIFT(line), 0);

	/* set filter */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + reg_offset,
			  0x7 << GPIO_IRQ_FILTER_SHIFT(line),
			  0x7 << GPIO_IRQ_FILTER_SHIFT(line));

	/* select trigger pin */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + reg_offset,
			  0x7f << bit_offset, irqNum << bit_offset);

	/* set trigger both type */
	if (flags & IRQF_TRIGGER_BOTH) {
		val |= GPIO_IRQ_BOTH_SHIFT(line);
		REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_EDGE_POL_EXTR,
				  GPIO_IRQ_BOTH_SHIFT(line), val);
		return;
	}

	/* set trigger single edge or level  type */
	if (flags & (IRQF_TRIGGER_LOW | IRQF_TRIGGER_FALLING))
		val |= GPIO_IRQ_POL_SHIFT(line);

	if (flags & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING))
		val |= GPIO_IRQ_EDGE_SHIFT(line);

	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE, REG_EDGE_POL_MASK_SC2(line), val);
}
