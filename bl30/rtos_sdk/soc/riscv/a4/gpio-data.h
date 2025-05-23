/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * platform related header file
 */

#ifndef _MESON_A4_GPIO_H_
#define _MESON_A4_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif
#define GPIO_NUM_MAX 81 /*actual numbers of pins*/
#define BANK_NUM_MAX 7 /*numbers of gpio bank*/
#define IRQ_REG_NUM 8 /* numbers of irq relative reg*/

#define GPIO_INVALID 0xffff

#define GPIO_AO_IRQ_BASE PADCTRL_GPIOAO_IRQ_CTRL0

#define GPIO_AO_EXT_IRQ_NUM	3

/* GPIOAO */
#define GPIOAO_0			0
#define GPIOAO_1			1
#define GPIOAO_2			2
#define GPIOAO_3			3
#define GPIOAO_4			4
#define GPIOAO_5			5
#define GPIOAO_6			6

#define	GPIO_TEST_N			32

/* GPIOE */
#define GPIOE_0				64
#define GPIOE_1				65

/* GPIOD */
#define GPIOD_0				96
#define GPIOD_1				97
#define GPIOD_2				98
#define GPIOD_3				99
#define GPIOD_4				100
#define GPIOD_5				101
#define GPIOD_6				102
#define GPIOD_7				103
#define GPIOD_8				104
#define GPIOD_9				105
#define GPIOD_10			106
#define GPIOD_11			107
#define GPIOD_12			108
#define GPIOD_13			109
#define GPIOD_14			110
#define GPIOD_15			111

/* GPIOB */
#define GPIOB_0				128
#define GPIOB_1				129
#define GPIOB_2				130
#define GPIOB_3				131
#define GPIOB_4				132
#define GPIOB_5				133
#define GPIOB_6				134
#define GPIOB_7				135
#define GPIOB_8				136
#define GPIOB_9				137
#define GPIOB_10			138
#define GPIOB_11			139
#define GPIOB_12			140
#define GPIOB_13			141

/* GPIOX */
#define GPIOX_0				160
#define GPIOX_1				161
#define GPIOX_2				162
#define GPIOX_3				163
#define GPIOX_4				164
#define GPIOX_5				165
#define GPIOX_6				166
#define GPIOX_7				167
#define GPIOX_8				168
#define GPIOX_9				169
#define GPIOX_10			170
#define GPIOX_11			171
#define GPIOX_12			172
#define GPIOX_13			173
#define GPIOX_14			174
#define GPIOX_15			175
#define GPIOX_16			176
#define GPIOX_17			177

/* GPIOT */
#define GPIOT_0				192
#define GPIOT_1				193
#define GPIOT_2				194
#define GPIOT_3				195
#define GPIOT_4				196
#define GPIOT_5				197
#define GPIOT_6				198
#define GPIOT_7				199
#define GPIOT_8				200
#define GPIOT_9				201
#define GPIOT_10			202
#define GPIOT_11			203
#define GPIOT_12			204
#define GPIOT_13			205
#define GPIOT_14			206
#define GPIOT_15			207
#define GPIOT_16			208
#define GPIOT_17			209
#define GPIOT_18			210
#define GPIOT_19			211
#define GPIOT_20			212
#define GPIOT_21			213
#define GPIOT_22			214

#ifdef __cplusplus
}
#endif
#endif
