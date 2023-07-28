/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * platform related header file
 */

#ifndef _MESON_S1A_GPIO_H_
#define _MESON_S1A_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif
#define GPIO_NUM_MAX 40 /*actual numbers of pins*/
#define BANK_NUM_MAX 5 /*numbers of gpio bank*/
#define IRQ_REG_NUM 8 /* numbers of irq relative reg*/

#define GPIO_INVALID 0xffff

/* GPIOZ */
#define GPIOZ_0				0
#define GPIOZ_1				1
#define GPIOZ_2				2
#define GPIOZ_3				3
#define GPIOZ_4				4
#define GPIOZ_5				5
#define GPIOZ_6				6
#define GPIOZ_7				7
#define GPIOZ_8				8
#define GPIOZ_9				9

/* GPIOH */
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

/* GPIOD */
#define GPIOD_0				64
#define GPIOD_1				65
#define GPIOD_2				66
#define GPIOD_3				67
#define GPIOD_4				68
#define GPIOD_5				69
#define GPIOD_6				70
#define GPIOD_7				71
#define GPIOD_8				72
#define GPIOD_9				73
#define GPIOD_10			74
#define GPIOD_11			75

/* GPIOB */
#define GPIOB_0				96
#define GPIOB_1				97
#define GPIOB_2				98
#define GPIOB_3				99
#define GPIOB_4				100
#define GPIOB_5				101

#define	GPIO_TEST_N			128

#endif
