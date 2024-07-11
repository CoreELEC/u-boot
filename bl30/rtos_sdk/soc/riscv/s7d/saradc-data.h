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

#define SAR_SYS_CLK_EN_BASE CLKCTRL_SYS_CLK_EN0_REG2
#define SAR_SYS_CLK_EN_BIT 28

#define SARADC_REG7_INIT 0x00000c11
#define SARADC_REG8_INIT 0x0280614d
#define SARADC_REG3_INIT 0x10a02403
#define SARADC_REG4_INIT 0x00000080
#define SARADC_REG9_INIT 0x0000e4e4
#define SARADC_REG10_INIT 0x74543414
#define SARADC_REG11_INIT 0xf4d4b494

/* s7d saradc interrupt num */
#define SARADC_INTERRUPT_NUM 181

#define SARADC_REG_NUM ((0x3c >> 2) + 2)

#endif
