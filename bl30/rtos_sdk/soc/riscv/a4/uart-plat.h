/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __UART_PLAT__
#define __UART_PLAT__

#include "register.h"

#define UART_AO_IRQ_ENABLE BIT(3)

static inline void UART_AO_WakeUp_Setting(void)
{
	uint32_t value;

	/* enable uart_ao irq */
	value = REG32(AO_IRQCTRL_MASK_PWRCTRL);
	value |= UART_AO_IRQ_ENABLE;
	REG32(AO_IRQCTRL_MASK_PWRCTRL) = value;

	/* enable uart_ao tick */
	value = REG32(AO_CLKCTRL_TIMEBASE_CTRL0);
	value |= BIT(18);
	REG32(AO_CLKCTRL_TIMEBASE_CTRL0) = value;

	value = REG32(AO_CLKCTRL_TIMEBASE_CTRL1);
	value |= BIT(0);
	REG32(AO_CLKCTRL_TIMEBASE_CTRL1) = value;
}
#endif
