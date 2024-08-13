/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
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
};

/* VDDEE voltage table  volt must ascending */
struct xPwmMesonVoltage vddee_table[] = {
	{ 730, 0x140000 }, { 740, 0x120000 }, { 750, 0x110001 }, { 760, 0x100002 },
	{ 770, 0x0f0003 }, { 780, 0x0e0004 }, { 790, 0x0d0005 }, { 800, 0x0c0006 },
	{ 810, 0x0b0007 }, { 820, 0x0a0008 }, { 830, 0x090009 }, { 840, 0x08000a },
	{ 850, 0x07000b }, { 860, 0x06000c }, { 870, 0x05000d }, { 880, 0x04000e },
	{ 890, 0x03000f }, { 900, 0x020010 }, { 910, 0x010011 }, { 920, 0x000012 },
	{ 930, 0x000014 },
};

/* VDDCPU voltage table  volt must ascending */
struct xPwmMesonVoltage vddcpu_table[] = {
	{ 690, 0x3E80000 },  { 700, 0x3D30013 },  { 710, 0x3B50031 },  { 720, 0x397004F },
	{ 730, 0x379006D },  { 740, 0x3650081 },  { 750, 0x347009F },  { 760, 0x32900BD },
	{ 770, 0x31500D1 },  { 780, 0x2F700EF },  { 790, 0x2D9010D },  { 800, 0x2BB012B },
	{ 810, 0x2A7013F },  { 820, 0x289015D },  { 830, 0x26B017B },  { 840, 0x24D0199 },
	{ 850, 0x22F01B7 },  { 860, 0x21B01CB },  { 870, 0x1FD01E9 },  { 880, 0x1DF0207 },
	{ 890, 0x1C10225 },  { 900, 0x1AD0239 },  { 910, 0x18F0257 },  { 920, 0x1710275 },
	{ 930, 0x1530293 },  { 940, 0x13502B1 },  { 950, 0x12102C5 },  { 960, 0x10302E3 },
	{ 970, 0xE50301 },  { 980, 0xC7031F },  { 990, 0xA9033D },  { 1000, 0x8B035B },
	{ 1010, 0x77036F }, { 1020, 0x59038D }, { 1030, 0x3B03AB }, { 1040, 0x1D03C9 },
	{ 1050, 0x3E8 },
};

/*
 * todo: need processing here vddee pwmh vddcpu pwmj
 * Different boards may use different pwm channels
 */
uint32_t prvMesonVoltToPwmchip(enum pwm_voltage_id voltage_id)
{
	switch (voltage_id) {
	case VDDEE_VOLT:
		return PWM_A;

	case VDDCPU_VOLT:
		return PWM_B;

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
	switch (voltage_id) {
	case VDDEE_VOLT:
		return MESON_PWM_0;

	case VDDCPU_VOLT:
		return MESON_PWM_0;

	default:
		break;
	}
	return MESON_PWM_2;
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
