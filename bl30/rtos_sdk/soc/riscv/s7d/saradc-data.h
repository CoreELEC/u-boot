/*
 * Copyright (c) 2021-2024 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SARADC_DATA_H_
#define _SARADC_DATA_H_

#include <register.h>

#define SAR_CLK_BASE CLKCTRL_SAR_CLK_CTRL0
#define SARADC_BASE SAR_ADC_REG0

/* s7d saradc interrupt num */
#define SARADC_INTERRUPT_NUM 181

#define SARADC_REG_NUM ((0x3c >> 2) + 1)

#endif
