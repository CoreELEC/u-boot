/*
 * Copyright (c) 2021-2024 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * platform related header file
 */

#ifndef _MESON_T6D_GPIO_H_
#define _MESON_T6D_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define	GPIO_NUM_MAX	129 /* actual numbers of pins */
#define	BANK_NUM_MAX	12  /* numbers of gpio bank */
#define	IRQ_REG_NUM	8   /* numbers of irq relative reg */

#define	GPIO_INVALID	0xffff

/* 0:31 */
#define GPIOW_0				0
#define GPIOW_1				1
#define GPIOW_2				2
#define GPIOW_3				3
#define GPIOW_4				4
#define GPIOW_5				5
#define GPIOW_6				6
#define GPIOW_7				7
#define GPIOW_8				8
#define GPIOW_9				9
#define GPIOW_10			10
#define GPIOW_11			11
#define GPIOW_12			12

/* 32:63 */
#define GPIOD_0				32
#define GPIOD_1				33
#define GPIOD_2				34
#define GPIOD_3				35
#define GPIOD_4				36
#define GPIOD_5				37
#define GPIOD_6				38
#define GPIOD_7				39
#define GPIOD_8				40
#define GPIOD_9				41
#define GPIOD_10			42
#define GPIOD_11			43
#define GPIOD_12			44
#define GPIOD_13			45
#define GPIOD_14			46

/* 64:95 */
#define GPIOE_0				64
#define GPIOE_1				65
#define GPIOE_2				66

/* 96:127 */
#define GPIOB_0				96
#define GPIOB_1				97
#define GPIOB_2				98
#define GPIOB_3				99
#define GPIOB_4				100
#define GPIOB_5				101
#define GPIOB_6				102
#define GPIOB_7				103
#define GPIOB_8				104
#define GPIOB_9				105
#define GPIOB_10			106
#define GPIOB_11			107
#define GPIOB_12			108
#define GPIOB_13			109

/* 128:159 */
#define GPIOC_0				128
#define GPIOC_1				129
#define GPIOC_2				130
#define GPIOC_3				131
#define GPIOC_4				132
#define GPIOC_5				133
#define GPIOC_6				134
#define GPIOC_7				135
#define GPIOC_8				136
#define GPIOC_9				137
#define GPIOC_10			138

/* 160:191 */
#define GPIOZ_0				160
#define GPIOZ_1				161
#define GPIOZ_2				162
#define GPIOZ_3				163
#define GPIOZ_4				164
#define GPIOZ_5				165
#define GPIOZ_6				166
#define GPIOZ_7				167
#define GPIOZ_8				168
#define GPIOZ_9				169
#define GPIOZ_10			170
#define GPIOZ_11			171
#define GPIOZ_12			172
#define GPIOZ_13			173
#define GPIOZ_14			174
#define GPIOZ_15			175
/* 192:223 */
#define GPIOZ_16			192
#define GPIOZ_17			193
#define GPIOZ_18			194
#define GPIOZ_19			195

/* 224:255 */
#define GPIOH_0				224
#define GPIOH_1				225
#define GPIOH_2				226
#define GPIOH_3				227
#define GPIOH_4				228
#define GPIOH_5				229
#define GPIOH_6				230
#define GPIOH_7				231
#define GPIOH_8				232
#define GPIOH_9				233
#define GPIOH_10			234
#define GPIOH_11			235
#define GPIOH_12			236
#define GPIOH_13			237
#define GPIOH_14			238
#define GPIOH_15			239
/* 256:287 */
#define GPIOH_16			256
#define GPIOH_17			257
#define GPIOH_18			258
#define GPIOH_19			259
#define GPIOH_20			260
#define GPIOH_21			261

/* 288:319 */
#define GPIOM_0				288
#define GPIOM_1				289
#define GPIOM_2				290
#define GPIOM_3				291
#define GPIOM_4				292
#define GPIOM_5				293
#define GPIOM_6				294
#define GPIOM_7				295
#define GPIOM_8				296
#define GPIOM_9				297
#define GPIOM_10			298
#define GPIOM_11			299
#define GPIOM_12			300
#define GPIOM_13			301
#define GPIOM_14			302
#define GPIOM_15			303
/* 320:351 */
#define GPIOM_16			320
#define GPIOM_17			321
#define GPIOM_18			322
#define GPIOM_19			323
#define GPIOM_20			324
#define GPIOM_21			325
#define GPIOM_22			326
#define GPIOM_23			327
#define GPIOM_24			328
#define GPIOM_25			329
#define GPIOM_26			330
#define GPIOM_27			331
#define GPIOM_28			332
#define GPIOM_29			333

/* 352:384 */
#define GPIO_TEST_N			352

#ifdef __cplusplus
}
#endif

#endif /* _MESON_T6D_GPIO_H_ */
