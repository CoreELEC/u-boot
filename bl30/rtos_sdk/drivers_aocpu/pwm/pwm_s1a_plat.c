/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include <common.h>
#include <pwm.h>

struct xPwmMesonChip meson_pwm_chip[] = {
	{ PWM_AB, PWMAB_PWM_A, 0, CLKCTRL_PWM_CLK_AB_CTRL },
	{ PWM_CD, PWMCD_PWM_A, 0, CLKCTRL_PWM_CLK_CD_CTRL },
};

/* VDDEE & VDDCPU voltage table  volt must ascending */
struct xPwmMesonVoltage vddee_table[] = {
	{ 799, 0x00220000 }, { 809, 0x00210001 }, { 819, 0x00200002 }, { 829, 0x001F0003 },
	{ 839, 0x001E0004 }, { 849, 0x001D0005 }, { 859, 0x001C0006 }, { 869, 0x001B0007 },
	{ 879, 0x001A0008 }, { 889, 0x00190009 }, { 899, 0x0018000A }, { 909, 0x0017000B },
	{ 919, 0x0016000D }, { 929, 0x0015000D }, { 939, 0x0014000E }, { 949, 0x0013000F },
	{ 959, 0x00120010 }, { 969, 0x00110011 }, { 979, 0x00100012 }, { 989, 0x000F0013 },
	{ 999, 0x000E0014 }, { 1009, 0x000D0015 }, { 1019, 0x000C0016 }, { 1029, 0x000B0017 },
	{ 1039, 0x000A0018 }, { 1049, 0x00090019 }, { 1059, 0x0008001A }, { 1069, 0x0007001B },
	{ 1079, 0x0006001C }, { 1089, 0x0005001D }, { 1099, 0x0004001E }, { 1109, 0x0003001F },
	{ 1119, 0x00020020 }, { 1129, 0x00010021 }, { 1139, 0x00000022 },
};

struct board_pwm_cfg_t {
	uint32_t pwm_ctrl;
	uint32_t pwm_chn;
	struct {
		struct xPwmMesonVoltage *table;
		uint32_t table_size;
	};
};

#define VOLTAGE_TABLE_INFO(x) {x, ARRAY_SIZE(x)}

struct board_pwm_cfg_t board_cfg[] = {
	[VDDEE_VOLT] = {PWM_AB, MESON_PWM_0, VOLTAGE_TABLE_INFO(vddee_table)},
	[VDDCPU_VOLT] = {PWM_AB, MESON_PWM_0, VOLTAGE_TABLE_INFO(vddee_table)},
};

/*
 * todo: need processing here vddee pwmh vddcpu pwmj
 * Different boards may use different pwm channels
 */
uint32_t prvMesonVoltToPwmchip(enum pwm_voltage_id voltage_id)
{
	if (!(voltage_id < MAX_ITEM_VOLT))
		return PWM_MUX;

	return board_cfg[voltage_id].pwm_ctrl;
}

/*
 * todo: need processing here
 * Different boards may use different pwm channels
 */
uint32_t prvMesonVoltToPwmchannel(enum pwm_voltage_id voltage_id)
{
	if (!(voltage_id < MAX_ITEM_VOLT))
		return MESON_PWM_2;

	return board_cfg[voltage_id].pwm_chn;
}

struct xPwmMesonVoltage *vPwmMesonGetVoltTable(uint32_t voltage_id)
{
	if (!(voltage_id < MAX_ITEM_VOLT))
		return NULL;

	return board_cfg[voltage_id].table;
}

uint32_t vPwmMesonGetVoltTableSize(uint32_t voltage_id)
{
	if (!(voltage_id < MAX_ITEM_VOLT))
		return 0;

	return board_cfg[voltage_id].table_size;
}

struct xPwmMesonChip *prvIdToPwmChip(uint32_t chip_id)
{
	if (chip_id >= PWM_MUX) {
		printf("pwm chip id is invail!\n");
		return NULL;
	}

	return meson_pwm_chip + chip_id;
}
