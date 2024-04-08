/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _MESON_T6D_GPIO_H_
#define _MESON_T6D_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "register.h"
#define GPIO_NUM_MAX 150 /*actual numbers of pins*/
#define BANK_NUM_MAX 13 /*numbers of gpio bank*/
#define IRQ_REG_NUM 8 /* numbers of irq relative reg*/

#define GPIO_INVALID 0xffff

/*0-31*/
#define GPIOD_0 0
#define GPIOD_1 1
#define GPIOD_2 2
#define GPIOD_3 3
#define GPIOD_4 4
#define GPIOD_5 5
#define GPIOD_6 6
#define GPIOD_7 7
#define GPIOD_8 8
#define GPIOD_9 9
#define GPIOD_10 10
#define GPIOD_11 11
#define GPIOD_12 12
#define GPIOD_13 13
#define GPIOD_14 14

/*32-63*/
#define GPIOE_0 32
#define GPIOE_1 33

/*64-95*/
#define GPIOZ_0 64
#define GPIOZ_1 65
#define GPIOZ_2 66
#define GPIOZ_3 67
#define GPIOZ_4 68
#define GPIOZ_5 69
#define GPIOZ_6 70
#define GPIOZ_7 71
#define GPIOZ_8 72
#define GPIOZ_9 73
#define GPIOZ_10 74
#define GPIOZ_11 75
#define GPIOZ_12 76
#define GPIOZ_13 77
#define GPIOZ_14 78
#define GPIOZ_15 79

/*96-127 */
#define GPIOZ_16 96
#define GPIOZ_17 97
#define GPIOZ_18 98
#define GPIOZ_19 99

/*128-159 */
#define GPIOH_0 128
#define GPIOH_1 129
#define GPIOH_2 130
#define GPIOH_3 131
#define GPIOH_4 132
#define GPIOH_5 133
#define GPIOH_6 134
#define GPIOH_7 135
#define GPIOH_8 136
#define GPIOH_9 137
#define GPIOH_10 138
#define GPIOH_11 139
#define GPIOH_12 140
#define GPIOH_13 141
#define GPIOH_14 142
#define GPIOH_15 143

/* 160-191 */
#define GPIOH_16 160
#define GPIOH_17 161
#define GPIOH_18 162
#define GPIOH_19 163
#define GPIOH_20 164
#define GPIOH_21 165
#define GPIOH_22 166
#define GPIOH_23 167
#define GPIOH_24 168
#define GPIOH_25 169
#define GPIOH_26 170
#define GPIOH_27 171
#define GPIOH_28 172
#define GPIOH_29 173

/* 192 */
#define GPIOB_0 192
#define GPIOB_1 193
#define GPIOB_2 194
#define GPIOB_3 195
#define GPIOB_4 196
#define GPIOB_5 197
#define GPIOB_6 198
#define GPIOB_7 199
#define GPIOB_8 200
#define GPIOB_9 201
#define GPIOB_10 202
#define GPIOB_11 203
#define GPIOB_12 204
#define GPIOB_13 205

/* 224 */
#define GPIOC_0 224
#define GPIOC_1 225
#define GPIOC_2 226
#define GPIOC_3 227
#define GPIOC_4 228
#define GPIOC_5 229
#define GPIOC_6 230
#define GPIOC_7 231
#define GPIOC_8 232
#define GPIOC_9 233
#define GPIOC_10 234

/* 256 */
#define GPIOP_0  256
#define GPIOP_1  257
#define GPIOP_2  258
#define GPIOP_3  259
#define GPIOP_4  260
#define GPIOP_5  261
#define GPIOP_6  262
#define GPIOP_7  263
#define GPIOP_8  264
#define GPIOP_9  265

/* 288 */
#define GPIOW_0 288
#define GPIOW_1 289
#define GPIOW_2 290
#define GPIOW_3 291
#define GPIOW_4 292
#define GPIOW_5 293
#define GPIOW_6 294
#define GPIOW_7 295
#define GPIOW_8 296
#define GPIOW_9 297
#define GPIOW_10 298
#define GPIOW_11 299
#define GPIOW_12 300
#define GPIOW_13 301
#define GPIOW_14 302
#define GPIOW_15 303

/* 320 */
#define GPIOW_16 320

/* 352 */
#define GPIOM_0 352
#define GPIOM_1 353
#define GPIOM_2 354
#define GPIOM_3 355
#define GPIOM_4 356
#define GPIOM_5 357
#define GPIOM_6 358
#define GPIOM_7 359
#define GPIOM_8 360
#define GPIOM_9 361
#define GPIOM_10 362
#define GPIOM_11 363
#define GPIOM_12 364
#define GPIOM_13 365
#define GPIOM_14 366
#define GPIOM_15 367
#define GPIOM_16 368
#define GPIOM_17 369
#define GPIOM_18 370
#define GPIOM_19 371
#define GPIOM_20 372
#define GPIOM_21 373
#define GPIOM_22 374
#define GPIOM_23 375
#define GPIOM_24 376
#define GPIOM_25 377
#define GPIOM_26 378
#define GPIOM_27 379
#define GPIOM_28 380
#define GPIOM_29 381

#define	GPIO_TEST_N 384

#ifdef __cplusplus
}
#endif
#endif /* _MESON_T6D_GPIO_H_ */
