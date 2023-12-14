/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include <register.h>
#include <common.h>
#include <gpio.h>
#include "projdefs.h"
#include "gpio_drv.h"
#include "gpio_irq.h"
#include "portmacro.h"

/* gpio irq controller EE */
#define IRQ_GPIO0_NUM 10
#define IRQ_GPIO1_NUM 11
#define IRQ_GPIO2_NUM 12
#define IRQ_GPIO3_NUM 13
#define IRQ_GPIO4_NUM 14
#define IRQ_GPIO5_NUM 15
#define IRQ_GPIO6_NUM 16
#define IRQ_GPIO7_NUM 17

/* gpio irq controller AO */
#define IRQ_AO_GPIO0_NUM 140
#define IRQ_AO_GPIO1_NUM 141

#define AO_GPIO_NUM 63
#define AO_IRQ_NUM_BASE 256	/* Just to discriminate between AO & EE GPIO */

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

static const struct GpioDomain aoDomain = {
	.name = "AO",
	.rPullen = PADCTRL_GPIOAO_I,
	.rPull = PADCTRL_GPIOAO_I,
	.rGpio = PADCTRL_GPIOAO_I,
	.rMux = PADCTRL_PIN_MUX_REGAO,
	.rDrv = PADCTRL_GPIOAO_I,
};

static const struct GpioDomain eeDomain = {
	.name = "EE",
	.rPullen = PADCTRL_GPIOZ_I,
	.rPull = PADCTRL_GPIOZ_I,
	.rGpio = PADCTRL_GPIOZ_I,
	.rMux = PADCTRL_PIN_MUX_REG0,
	.rDrv = PADCTRL_GPIOZ_I,
};

static const struct GpioBank gpioBanks[BANK_NUM_MAX] = {
	/* name   pin_num   domain   pullen   pull   dir   out   in   mux   drv  */
	BANK_V2("AO",  7, &aoDomain, 0x03, 0, 0x04, 0, 0x02, 0, 0x01, 0, 0x00, 0,
		0x00, 0, 0x07, 0),
	BANK_V2("TEST_N", 1, &aoDomain, 0x13, 0, 0x14, 0, 0x12, 0, 0x11, 0, 0x10, 0,
		0x00, 28, 0x17, 0),
	BANK_V2("E",   2, &eeDomain, 0x43, 0, 0x44, 0, 0x42, 0, 0x41, 0, 0x40, 0,
		0x12, 0, 0x47, 0),
	BANK_V2("D",  16, &eeDomain, 0x33, 0, 0x34, 0, 0x32, 0, 0x31, 0, 0x30, 0,
		0x10, 0, 0x37, 0),
	BANK_V2("B",  14, &eeDomain, 0x63, 0, 0x64, 0, 0x62, 0, 0x61, 0, 0x60, 0,
		0x00, 0, 0x67, 0),
	BANK_V2("X",  18, &eeDomain, 0x13, 0, 0x14, 0, 0x12, 0, 0x11, 0, 0x10, 0,
		0x03, 0, 0x17, 0),
	BANK_V2("T",  23, &aoDomain, 0x23, 0, 0x24, 0, 0x22, 0, 0x21, 0, 0x20, 0,
		0x0b, 0, 0x27, 0),
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

static struct ParentIRQDesc aoIRQs[] = {
	[GPIO_AO_IRQ_L0] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_AO_GPIO0_NUM),
	[GPIO_AO_IRQ_L1] = PARENT_IRQ_BK(NULL, 0, GPIO_INVALID, IRQ_AO_GPIO1_NUM),

};

static const struct GpioIRQBank irqBanks[BANK_NUM_MAX] = {
	GPIO_IRQ_BK("AO", AO_IRQ_NUM_BASE + 0, aoIRQs, ARRAY_SIZE(aoIRQs)),
	GPIO_IRQ_BK("TEST_N", AO_IRQ_NUM_BASE + 7, aoIRQs, ARRAY_SIZE(aoIRQs)),
	GPIO_IRQ_BK("E", 14, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("D", 16, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("B", 0, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("X", 55, eeIRQs, ARRAY_SIZE(eeIRQs)),
	GPIO_IRQ_BK("T", 32, eeIRQs, ARRAY_SIZE(eeIRQs)),
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
	uint32_t reg_irq_base = 0;
	uint32_t reg_edge_base = 0;

	if (irqNum > AO_IRQ_NUM_BASE) {
		irqNum -= AO_IRQ_NUM_BASE;
		reg_irq_base = GPIO_AO_IRQ_BASE;
		reg_edge_base = 0x08;
	} else {
		reg_irq_base = GPIO_EE_IRQ_BASE;
		reg_edge_base = REG_EDGE_POL_EXTR;
	}

	bit_offset = ((line % 2) == 0) ? 0 : 16;
	reg_offset = REG_PIN_SC2_SEL + ((line / 2) << 2);

	/* clear both edge */
	REG32_UPDATE_BITS(reg_irq_base + reg_edge_base, GPIO_IRQ_BOTH_SHIFT(line), 0);

	/* set filter */
	REG32_UPDATE_BITS(reg_irq_base + reg_offset, 0x7 << GPIO_IRQ_FILTER_SHIFT(line),
			  0x7 << GPIO_IRQ_FILTER_SHIFT(line));

	/* select trigger pin */
	REG32_UPDATE_BITS(reg_irq_base + reg_offset, 0x7f << bit_offset, irqNum << bit_offset);

	/* set trigger both type */
	if (flags & IRQF_TRIGGER_BOTH) {
		val |= GPIO_IRQ_BOTH_SHIFT(line);
		REG32_UPDATE_BITS(reg_irq_base + reg_edge_base, GPIO_IRQ_BOTH_SHIFT(line),
				  val);
		return;
	}

	/* set trigger single edge or level  type */
	if (flags & (IRQF_TRIGGER_LOW | IRQF_TRIGGER_FALLING))
		val |= GPIO_IRQ_POL_SHIFT(line);

	if (flags & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING))
		val |= GPIO_IRQ_EDGE_SHIFT(line);

	REG32_UPDATE_BITS(reg_irq_base, REG_EDGE_POL_MASK_SC2(line), val);
}
