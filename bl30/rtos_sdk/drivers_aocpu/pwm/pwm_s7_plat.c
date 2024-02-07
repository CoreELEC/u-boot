/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * pwm s4 plat driver
 */
#include "FreeRTOS.h"
#include <stdio.h>
#include <common.h>
#include <pwm.h>

struct xPwmMesonChip meson_pwm_chip[] = {
	{ PWM_A, PWM_PWM_A, 0, CLKCTRL_PWM_CLK_AB_CTRL, pdTRUE, pdTRUE},
	{ PWM_B, PWM_PWM_B, 0, CLKCTRL_PWM_CLK_AB_CTRL, pdTRUE },
	{ PWM_C, PWM_PWM_C, 0, CLKCTRL_PWM_CLK_CD_CTRL, pdTRUE, pdTRUE},
	{ PWM_D, PWM_PWM_D, 0, CLKCTRL_PWM_CLK_CD_CTRL, pdTRUE },
	{ PWM_E, PWM_PWM_E, 0, CLKCTRL_PWM_CLK_EF_CTRL, pdTRUE, pdTRUE},
	{ PWM_F, PWM_PWM_F, 0, CLKCTRL_PWM_CLK_EF_CTRL, pdTRUE },
	{ PWM_G, PWM_PWM_G, 0, CLKCTRL_PWM_CLK_GH_CTRL, pdTRUE, pdTRUE},
	{ PWM_H, PWM_PWM_H, 0, CLKCTRL_PWM_CLK_GH_CTRL, pdTRUE },
	{ PWM_I, PWM_PWM_I, 0, CLKCTRL_PWM_CLK_IJ_CTRL, pdTRUE, pdTRUE},
	{ PWM_J, PWM_PWM_J, 0, CLKCTRL_PWM_CLK_IJ_CTRL, pdTRUE, },
};

struct xPwmMesonVoltage vddee_table[] = {
	{ 700, 0x120000 }, { 710, 0x110001 }, { 720, 0x100002 }, { 730, 0xf0003 },
	{ 740, 0xe0004 }, { 750, 0xd0005 }, { 760, 0xc0006 }, { 770, 0xb0007 },
	{ 780, 0xa0008 }, { 790, 0x90009 }, { 800, 0x8000a }, { 810, 0x7000b },
	{ 820, 0x6000c }, { 830, 0x5000d }, { 840, 0x4000e }, { 850, 0x3000f },
	{ 860, 0x20010 }, { 870, 0x10011 }, { 880, 0x12 },
};
/* VDDCPU voltage table  volt must ascending */
struct xPwmMesonVoltage vddcpu_table[] = {
	{1049000, 0}, {1039000, 3}, {1029000, 6}, {1019000, 9},
	{1009000, 12}, {999000, 14}, {989000, 17}, {979000, 20},
	{969000, 23}, {959000, 26}, {949000, 29}, {939000, 31},
	{929000, 34}, {919000, 37}, {909000, 40}, {899000, 43},
	{889000, 45}, {879000, 48}, {869000, 51}, {859000, 54},
	{849000, 56}, {839000, 59}, {829000, 62}, {819000, 65},
	{809000, 68}, {799000, 70}, {789000, 73}, {779000, 76},
	{769000, 79}, {759000, 81}, {749000, 84}, {739000, 87},
	{729000, 89}, {719000, 92}, {709000, 95}, {699000, 98},
	{689000, 100},
};

/*
 * todo: need processing here vddee pwmh vddcpu pwmj
 * Different boards may use different pwm channels
 */
uint32_t prvMesonVoltToPwmchip(enum pwm_voltage_id voltage_id)
{
	switch (voltage_id) {
	case VDDEE_VOLT:
		return PWM_H;

	case VDDCPU_VOLT:
		return PWM_J;

	default:
		break;
	}
	return PWM_MUX;
}

/*
 * todo: need processing here
 * Different boards may use different pwm channels
 */
uint32_t prvMesonVoltToPwmchannel(enum pwm_voltage_id voltage_id)
{
	return MESON_PWM_0;
}

struct xPwmMesonVoltage *vPwmMesonGetVoltTable(uint32_t voltage_id)
{
	switch (voltage_id) {
	case VDDEE_VOLT:
		return vddee_table;

	case VDDCPU_VOLT:
		return vddcpu_table;

	default:
		break;
	}
	return NULL;
}

uint32_t vPwmMesonGetVoltTableSize(uint32_t voltage_id)
{
	switch (voltage_id) {
	case VDDEE_VOLT:
		return sizeof(vddee_table) / sizeof(struct xPwmMesonVoltage);

	case VDDCPU_VOLT:
		return sizeof(vddcpu_table) / sizeof(struct xPwmMesonVoltage);

	default:
		break;
	}
	return 0;
}

struct xPwmMesonChip *prvIdToPwmChip(uint32_t chip_id)
{
	if (chip_id >= PWM_MUX) {
		printf("pwm chip id is invail!\n");
		return NULL;
	}

	return meson_pwm_chip + chip_id;
}
