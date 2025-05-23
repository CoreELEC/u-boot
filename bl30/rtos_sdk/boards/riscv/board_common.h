/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BOARD_COMMON_H__
#define __BOARD_COMMON_H__

#define GE_GPIO_CTRL(n, io, is_invert)	\
static inline void n##_on(void) \
{							\
	int ret;					\
	printf("%s on, invert:%d\n", #n, is_invert);	\
	ret = xGpioSetDir(io, GPIO_DIR_OUT);		\
	if (ret < 0) {					\
		printf("%s gpio dir fail\n", #n);	\
		return;					\
	}						\
							\
	if (is_invert)					\
		ret = xGpioSetValue(io, GPIO_LEVEL_LOW);	\
	else						\
		ret = xGpioSetValue(io, GPIO_LEVEL_HIGH);	\
	if (ret < 0) {					\
		printf("%s set gpio val fail\n", #n);	\
		return;					\
	}						\
}							\
							\
static inline void n##_off(void) \
{							\
	int ret;					\
	printf("%s off, invert:%d\n", #n, is_invert);	\
	ret = xGpioSetDir(io, GPIO_DIR_OUT);		\
	if (ret < 0) {					\
		printf("%s gpio dir fail\n", #n);	\
		return;					\
	}						\
							\
	if (is_invert)					\
		ret = xGpioSetValue(io, GPIO_LEVEL_HIGH);	\
	else						\
		ret = xGpioSetValue(io, GPIO_LEVEL_LOW);	\
	if (ret < 0) {					\
		printf("%s set gpio val fail\n", #n);	\
		return;					\
	}						\
}

#define HIZ_GPIO_CTRL(n, io)	\
static inline void n##_on(void)	\
{							\
	int ret;					\
	ret = xGpioSetDir(io, GPIO_DIR_IN);		\
	if (ret < 0) {					\
		printf("%s gpio dir fail\n", #n);	\
		return;					\
	}						\
}							\
							\
static inline void n##_off(void)			\
{							\
	int ret;					\
	ret = xGpioSetDir(io, GPIO_DIR_OUT);		\
	if (ret < 0) {					\
		printf("%s gpio dir fail\n", #n);	\
		return;					\
	}						\
							\
}
#define INVERT		1
#define NOINVERT	0

#endif /* __BOARD_COMMON_H__ */
