/*
 * Copyright (c) 2021-2023 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * platform related header file
 */

#ifndef _MESON_S7D_GPIO_H_
#define _MESON_S7D_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define	GPIO_NUM_MAX	82 /* actual numbers of pins */
#define	BANK_NUM_MAX	9  /* numbers of gpio bank */
#define	IRQ_REG_NUM	8  /* numbers of irq relative reg */

#define	GPIO_INVALID	0xffff

/* 0:31 */
#define	GPIOE_0		0
#define	GPIOE_1		1

/* 32:63 */
#define	GPIOB_0		32
#define	GPIOB_1		33
#define	GPIOB_2		34
#define	GPIOB_3		35
#define	GPIOB_4		36
#define	GPIOB_5		37
#define	GPIOB_6		38
#define	GPIOB_7		39
#define	GPIOB_8		40
#define	GPIOB_9		41
#define	GPIOB_10	42
#define	GPIOB_11	43
#define	GPIOB_12	44
#define	GPIOB_13	45

/* 64:95 */
#define	GPIOC_0		64
#define	GPIOC_1		65
#define	GPIOC_2		66
#define	GPIOC_3		67
#define	GPIOC_4		68
#define	GPIOC_5		69
#define	GPIOC_6		70
#define	GPIOC_7		71

/* 96:127 */
#define	GPIOD_0		96
#define	GPIOD_1		97
#define	GPIOD_2		98
#define	GPIOD_3		99
#define	GPIOD_4		100

/* 128:159 */
#define	GPIODV_0	128
#define	GPIODV_1	129
#define	GPIODV_2	130
#define	GPIODV_3	131
#define	GPIODV_4	132
#define	GPIODV_5	133
#define	GPIODV_6	134

/* 160:191 */
#define	GPIOH_0		160
#define	GPIOH_1		161
#define	GPIOH_2		162
#define	GPIOH_3		163
#define	GPIOH_4		164
#define	GPIOH_5		165
#define	GPIOH_6		166
#define	GPIOH_7		167
#define	GPIOH_8		168
#define	GPIOH_9		169
#define	GPIOH_10	170
#define	GPIOH_11	171

/* 192:223 */
#define	GPIOX_0		192
#define	GPIOX_1		193
#define	GPIOX_2		194
#define	GPIOX_3		195
#define	GPIOX_4		196
#define	GPIOX_5		197
#define	GPIOX_6		198
#define	GPIOX_7		199
#define	GPIOX_8		200
#define	GPIOX_9		201
#define	GPIOX_10	202
#define	GPIOX_11	203
#define	GPIOX_12	204
#define	GPIOX_13	205
#define	GPIOX_14	206
#define	GPIOX_15	207
#define	GPIOX_16	208
#define	GPIOX_17	209
#define	GPIOX_18	210
#define	GPIOX_19	211

/* 224:255 */
#define	GPIOZ_0		224
#define	GPIOZ_1		225
#define	GPIOZ_2		226
#define	GPIOZ_3		227
#define	GPIOZ_4		228
#define	GPIOZ_5		229
#define	GPIOZ_6		230
#define	GPIOZ_7		231
#define	GPIOZ_8		232
#define	GPIOZ_9		233
#define	GPIOZ_10	234
#define	GPIOZ_11	235
#define	GPIOZ_12	236

/* 256:256 */
#define	GPIO_TEST_N	256

#ifdef __cplusplus
}
#endif

#endif /* _MESON_S7D_GPIO_H_ */
