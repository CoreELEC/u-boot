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
	{ 680, 0xf0000 },  { 690, 0xd0000 },  { 700, 0xc0001 },  { 710, 0xb0002 },
	{ 720, 0xa0003 },  { 730, 0x90004 },  { 740, 0x80005 },  { 750, 0x70006 },
	{ 760, 0x60007 },  { 770, 0x50008 },  { 780, 0x40009 },  { 790, 0x3000a },
	{ 800, 0x2000b },  { 810, 0x1000c },  { 820, 0x0000d },  { 830, 0x0000f },
};

/* VDDCPU voltage table  volt must ascending */
struct xPwmMesonVoltage vddcpu_table[] = {
	{ 690, 0x1e00000 },  { 690, 0x1c00000 },  { 700, 0x1b00001 },  { 710, 0x1a00002 },
	{ 720, 0x1900003 },  { 730, 0x1800004 },  { 740, 0x1700005 },  { 750, 0x1600006 },
	{ 760, 0x1500007 },  { 770, 0x1400008 },  { 780, 0x1300009 },  { 790, 0x120000a },
	{ 800, 0x110000b },  { 810, 0x100000c },  { 820, 0x00f000d },  { 830, 0x00e000e },
	{ 840, 0x00d000f },  { 850, 0x00c0010 },  { 860, 0x00b0011 },  { 870, 0x00a0012 },
	{ 880, 0x0090013 },  { 890, 0x0080014 },  { 900, 0x0070015 },  { 910, 0x0060016 },
	{ 920, 0x0050017 },  { 930, 0x0040018 },  { 940, 0x0030019 },  { 950, 0x002001a },
	{ 960, 0x001001b },  { 970, 0x000001c },  { 980, 0x000001e },
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
