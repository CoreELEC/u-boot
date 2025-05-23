/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include <common.h>
#include <pwm.h>
#include <secure_apb.h>

struct xPwmMesonChip meson_pwm_chip[] = {
	{ PWM_AB, PWM_PWM_A, 0, 0 },
	{ PWM_CD, PWM_PWM_C, 0, 0 },
	{ PWM_EF, PWM_PWM_E, 0, 0 },
	{ PWM_AO_AB, AO_PWM_PWM_A, 0, 0},
	{ PWM_AO_CD, AO_PWM_PWM_C, 0, 0},
};

/* VDDEE voltage table  volt must ascending */
struct xPwmMesonVoltage vddee_table[] = {
	{ 800, 0x0001D0000 }, { 810, 0x0001C0000 }, { 820, 0x0001B0001 }, { 830, 0x0001A0002 },
	{ 840, 0x000190003 }, { 850, 0x000180004 }, { 860, 0x000170005 }, { 870, 0x000160006 },
	{ 880, 0x000150007 }, { 890, 0x000140008 }, { 900, 0x000130009 }, { 910, 0x00012000A },
	{ 920, 0x00011000B }, { 930, 0x00010000C }, { 940, 0x0000F000D }, { 950, 0x0000E000E },
	{ 960, 0x0000D000F }, { 970, 0x0000C0010 }, { 980, 0x0000B0011 }, { 990, 0x0000A0012 },
	{ 1000, 0x000090013 }, { 1010, 0x000080014 }, { 1020, 0x000070015 }, { 1030, 0x000060016 },
	{ 1040, 0x000050017 }, { 1050, 0x000040018 }, { 1060, 0x000030019 }, { 1070, 0x00002001A },
	{ 1080, 0x00001001B }, { 1090, 0x00000001C }, { 1100, 0x00000001F },
};

/* VDDCPU voltage table  volt must ascending */
struct xPwmMesonVoltage vddcpu_table[] = {
	{ 800, 0x00220000 },  { 810, 0x00210001 },  { 820, 0x00200002 },  { 830, 0x001f0003 },
	{ 840, 0x001e0004 },  { 850, 0x001d0005 },  { 860, 0x001c0006 },  { 870, 0x001b0007 },
	{ 880, 0x001a0008 },  { 890, 0x00190009 },  { 900, 0x0018000a },  { 910, 0x0017000b },
	{ 920, 0x0016000c },  { 930, 0x0015000d },  { 940, 0x0014000e },  { 950, 0x0013000f },
	{ 960, 0x00120010 },  { 970, 0x00110011 },  { 980, 0x00100012 },  { 990, 0x000f0013 },
	{ 1000, 0x000e0014 },  { 1010, 0x000d0015 },  { 1020, 0x000c0016 },  { 1030, 0x000b0017 },
	{ 1040, 0x000a0018 },  { 1050, 0x00090019 },  { 1060, 0x0008001a },  { 1070, 0x0007001b },
	{ 1080, 0x0006001c },  { 1090, 0x0005001d },  { 1100, 0x0004001e },  { 1110, 0x0003001f },
	{ 1120, 0x00020020 },  { 1130, 0x00010021 },  { 1140, 0x00000022 },
};

/*
 * todo: need processing here vddee pwmh vddcpu pwmj
 * Different boards may use different pwm channels
 */
uint32_t prvMesonVoltToPwmchip(enum pwm_voltage_id voltage_id)
{
	switch (voltage_id) {
	case VDDEE_VOLT:
		return PWM_AO_AB;

	case VDDCPU_VOLT:
		return PWM_AB;

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
		return MESON_PWM_1;

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
