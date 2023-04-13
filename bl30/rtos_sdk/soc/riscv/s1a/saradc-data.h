/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SARADC_DATA_H_
#define _SARADC_DATA_H_

#include <register.h>

#define SAR_CLK_BASE		CLKCTRL_SAR_CLK_CTRL0
#define SARADC_BASE		SAR_ADC_REG0

#define REG11_VREF_EN_VALUE	0
#define REG11_CMV_SEL_VALUE	0
#define REG11_EOC_VALUE		1

/* s1a saradc interrupt num */
#define SARADC_INTERRUPT_NUM	181

#define SARADC_REG_NUM		(18 + 1)

#endif
