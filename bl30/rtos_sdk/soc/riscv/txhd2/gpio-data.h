/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _MESON_TXHD2_GPIO_H_
#define _MESON_TXHD2_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "register.h"

#define GPIO_NUM_MAX			80	/* actual numbers of pins */
#define BANK_NUM_MAX			7	/* numbers of gpio bank */
#define IRQ_REG_NUM			2	/* numbers of irq relative reg */
#define GPIO_INVALID			0xffff

#ifdef AO_IRQ_GPIO_REG
#define GPIO_AO_IRQ_BASE AO_IRQ_GPIO_REG
#endif

#define GPIOAO_PIN_NUM			13

/* GPIOAO [13:0] */
#define GPIOAO_IRQ_NUM_BASE		0
#define GPIOAO_0			0
#define GPIOAO_1			1
#define GPIOAO_2			2
#define GPIOAO_3			3
#define GPIOAO_4			4
#define GPIOAO_5			5
#define GPIOAO_6			6
#define GPIOAO_7			7
#define GPIOAO_8			8
#define GPIOAO_9			9
#define GPIOAO_10			10
#define GPIOAO_11			11
#define GPIOAO_12			12
#define GPIOAO_13			13

/* GPIOH [29:14] */
#define GPIOH_IRQ_NUM_BASE		14
#define GPIOH_0				32
#define GPIOH_1				33
#define GPIOH_2				34
#define GPIOH_3				35
#define GPIOH_4				36
#define GPIOH_5				37
#define GPIOH_6				38
#define GPIOH_7				39
#define GPIOH_8				40
#define GPIOH_9				41
#define GPIOH_10			42
#define GPIOH_11			43
#define GPIOH_12			44
#define GPIOH_13			45
#define GPIOH_14			46
#define GPIOH_15			47

/* GPIOB [42:30] */
#define GPIOB_IRQ_NUM_BASE		30
#define GPIOB_0				64
#define GPIOB_1				65
#define GPIOB_2				66
#define GPIOB_3				67
#define GPIOB_4				68
#define GPIOB_5				69
#define GPIOB_6				70
#define GPIOB_7				71
#define GPIOB_8				72
#define GPIOB_9				73
#define GPIOB_10			74
#define GPIOB_11			75
#define GPIOB_12			76

/* GPIOZ [50:43] */
#define GPIOZ_IRQ_NUM_BASE		43
#define GPIOZ_0				96
#define GPIOZ_1				97
#define GPIOZ_2				98
#define GPIOZ_3				99
#define GPIOZ_4				100
#define GPIOZ_5				101
#define GPIOZ_6				102
#define GPIOZ_7				103

/* GPIOW [58:51] */
#define GPIOW_IRQ_NUM_BASE		51
#define GPIOW_0				128
#define GPIOW_1				129
#define GPIOW_2				130
#define GPIOW_3				131
#define GPIOW_4				132
#define GPIOW_5				133
#define GPIOW_6				134
#define GPIOW_7				135

/* GPIOC [69:59] */
#define GPIOC_IRQ_NUM_BASE		59
#define GPIOC_0				160
#define GPIOC_1				161
#define GPIOC_2				162
#define GPIOC_3				163
#define GPIOC_4				164
#define GPIOC_5				165
#define GPIOC_6				166
#define GPIOC_7				167
#define GPIOC_8				168
#define GPIOC_9				169
#define GPIOC_10			170

/* GPIODV [79:70] */
#define GPIODV_IRQ_NUM_BASE		70
#define GPIODV_0			192
#define GPIODV_1			193
#define GPIODV_2			194
#define GPIODV_3			195
#define GPIODV_4			196
#define GPIODV_5			197
#define GPIODV_6			198
#define GPIODV_7			199
#define GPIODV_8			200
#define GPIODV_9			201

#ifdef __cplusplus
}
#endif

#endif /* _MESON_TXHD2_GPIO_H_ */
