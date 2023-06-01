/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SARADC_DATA_H_
#define _SARADC_DATA_H_

#include <register.h>

#define SAR_CLK_BASE AO_SAR_CLK
#define SARADC_BASE AO_SAR_ADC_REG0

#define REG11_VREF_EN_VALUE 1
#define REG11_CMV_SEL_VALUE 1
#define REG11_EOC_VALUE 1

/* txhd2 saradc interrupt num */
#define SARADC_INTERRUPT_NUM 8

#define SARADC_REG_NUM (18 + 1)

/* bandgap of gxlx3 and txhd2 is bit12 */
#define SARADC_REG11_BANDGAP_EN_BIT 12

#endif
