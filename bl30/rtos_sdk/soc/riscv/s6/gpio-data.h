/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * platform related header file
 */

#ifndef _MESON_S6_GPIO_H_
#define _MESON_S6_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif
#define GPIO_NUM_MAX 120 /*actual numbers of pins*/
#define BANK_NUM_MAX 10 /*numbers of gpio bank*/
#define IRQ_REG_NUM 20 /* numbers of irq relative reg*/

#define GPIO_INVALID 0xffff

/*0-31*/
#define	GPIOA_0		0
#define	GPIOA_1		1
#define	GPIOA_2		2
#define	GPIOA_3		3
#define	GPIOA_4		4
#define	GPIOA_5		5
#define	GPIOA_6		6
#define	GPIOA_7		7
#define	GPIOA_8		8
#define	GPIOA_9		9
#define	GPIOA_10	10

/*32-63*/
#define	GPIOC_0		32
#define	GPIOC_1		33
#define	GPIOC_2		34
#define	GPIOC_3		35
#define	GPIOC_4		36
#define	GPIOC_5		37
#define	GPIOC_6		38
#define	GPIOC_7		39

/*64-95*/
#define	GPIOD_0		64
#define	GPIOD_1		65
#define	GPIOD_2		66
#define	GPIOD_3		67
#define	GPIOD_4		68
#define	GPIOD_5		69
#define	GPIOD_6		70
#define	GPIOD_7		71
#define	GPIOD_8		72
#define	GPIOD_9		73
#define	GPIOD_10	74
#define	GPIOD_11	75

/*96-127 */
#define	GPIOE_0		96
#define	GPIOE_1		97
#define	GPIOE_2		98
#define	GPIOE_3		99
#define	GPIOE_4		100

/*128-159 */
#define	GPIOH_0		128
#define	GPIOH_1		129
#define	GPIOH_2		130
#define	GPIOH_3		131
#define	GPIOH_4		132
#define	GPIOH_5		133
#define	GPIOH_6		134
#define	GPIOH_7		135
#define	GPIOH_8		136

/* 160-191 */
#define	GPIOB_0		160
#define	GPIOB_1		161
#define	GPIOB_2		162
#define	GPIOB_3		163
#define	GPIOB_4		164
#define	GPIOB_5		165
#define	GPIOB_6		166
#define	GPIOB_7		167
#define	GPIOB_8		168
#define	GPIOB_9		169
#define	GPIOB_10	170
#define	GPIOB_11	171
#define	GPIOB_12	172

/* 192- 223 */
#define	GPIOT_0		192
#define	GPIOT_1		193
#define	GPIOT_2		194
#define	GPIOT_3		195
#define	GPIOT_4		196
#define	GPIOT_5		197
#define	GPIOT_6		198
#define	GPIOT_7		199
#define	GPIOT_8		200
#define	GPIOT_9		201
#define	GPIOT_10	202
#define	GPIOT_11	203
#define	GPIOT_12	204
#define	GPIOT_13	205
#define	GPIOT_14	206
#define	GPIOT_15	207
#define	GPIOT_16	208
#define	GPIOT_17	209
#define	GPIOT_18	210
#define	GPIOT_19	211
#define	GPIOT_20	212
#define	GPIOT_21	213
#define	GPIOT_22	214
#define	GPIOT_23	215
#define	GPIOT_24	216

/* 224- 255*/
#define	GPIOX_0		224
#define	GPIOX_1		225
#define	GPIOX_2		226
#define	GPIOX_3		227
#define	GPIOX_4		228
#define	GPIOX_5		229
#define	GPIOX_6		230
#define	GPIOX_7		231
#define	GPIOX_8		232
#define	GPIOX_9		233
#define	GPIOX_10	234
#define	GPIOX_11	235
#define	GPIOX_12	236
#define	GPIOX_13	237
#define	GPIOX_14	238
#define	GPIOX_15	239
#define	GPIOX_16	240
#define	GPIOX_17	241
#define	GPIOX_18	242
#define	GPIOX_19	243

/* 256- 287*/
#define	GPIOZ_0		256
#define	GPIOZ_1		257
#define	GPIOZ_2		258
#define	GPIOZ_3		259
#define	GPIOZ_4		260
#define	GPIOZ_5		261
#define	GPIOZ_6		262
#define	GPIOZ_7		263
#define	GPIOZ_8		264
#define	GPIOZ_9		265
#define	GPIOZ_10	266
#define	GPIOZ_11	267
#define	GPIOZ_12	268
#define	GPIOZ_13	269
#define	GPIOZ_14	270
#define	GPIOZ_15	271

/* 288- */
#define	GPIO_TEST_N	288

#ifdef __cplusplus
}
#endif
#endif /* _MESON_6_GPIO_H_ */
