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

#if defined(CONFIG_BOARD_AX208_S928X) || defined(CONFIG_BOARD_AX209_S928X)
/* VDDEE voltage table  volt must ascending */
struct xPwmMesonVoltage vddee_table[] = {
	{687, 0x00120000}, {694, 0x00110001}, {705, 0x00100002}, {715, 0x000f0003},
	{723, 0x000e0004}, {730, 0x000d0005}, {742, 0x000c0006}, {751, 0x000b0007},
	{759, 0x000a0008}, {768, 0x00090009}, {778, 0x0008000a}, {787, 0x0007000b},
	{796, 0x0006000c}, {805, 0x0005000d}, {814, 0x0004000e}, {823, 0x0003000f},
	{832, 0x00020010}, {841, 0x00010011}, {850, 0x00000012},
};

#else
/* VDDEE voltage table  volt must ascending */
struct xPwmMesonVoltage vddee_table[] = {
	{718, 0x00120000}, {727, 0x00110001}, {737, 0x00100002}, {747, 0x000f0003},
	{757, 0x000e0004}, {766, 0x000d0005}, {777, 0x000c0006}, {786, 0x000b0007},
	{797, 0x000a0008}, {806, 0x00090009}, {816, 0x0008000a}, {827, 0x0007000b},
	{836, 0x0006000c}, {847, 0x0005000d}, {856, 0x0004000e}, {866, 0x0003000f},
	{876, 0x00020010}, {886, 0x00010011}, {897, 0x00000012},
};

//#else
//#error "no found board"
#endif

/* VDDCPU voltage table  volt must ascending */
struct xPwmMesonVoltage vddcpu_table[] = {
	{692, 0x00220000}, {701, 0x00210001}, {712, 0x00200002}, {722, 0x001f0003},
	{731, 0x001e0004}, {742, 0x001d0005}, {751, 0x001c0006}, {761, 0x001b0007},
	{771, 0x001a0008}, {781, 0x00190009}, {791, 0x0018000a}, {798, 0x0017000b},
	{808, 0x0016000c}, {820, 0x0015000d}, {830, 0x0014000e}, {840, 0x0013000f},
	{850, 0x00120010}, {860, 0x00110011}, {870, 0x00100012}, {879, 0x000f0013},
	{889, 0x000e0014}, {900, 0x000d0015}, {909, 0x000c0016}, {919, 0x000b0017},
	{929, 0x000a0018}, {939, 0x00090019}, {948, 0x0008001a}, {957, 0x0007001b},
	{969, 0x0006001c}, {978, 0x0005001d}, {990, 0x0004001e}, {1000, 0x0003001f},
	{1008, 0x00020020}, {1019, 0x00010021}, {1029, 0x00000022},
};

struct xPwmMesonVoltage vdddos_npu_vpu_table[] = {
	{706, 0x00120000}, {717, 0x00110001}, {726, 0x00100002}, {734, 0x000f0003},
	{745, 0x000e0004}, {753, 0x000d0005}, {761, 0x000c0006}, {771, 0x000b0007},
	{780, 0x000a0008}, {789, 0x00090009}, {798, 0x0008000a}, {807, 0x0007000b},
	{817, 0x0006000c}, {826, 0x0005000d}, {835, 0x0004000e}, {844, 0x0003000f},
	{853, 0x00020010}, {863, 0x00010011}, {871, 0x00000012},
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
	[VDDEE_VOLT] = {PWM_AB, MESON_PWM_1, VOLTAGE_TABLE_INFO(vddee_table)},
	[VDDCPU_VOLT] = {PWM_AB, MESON_PWM_0, VOLTAGE_TABLE_INFO(vddcpu_table)},
	[VDDDOS_NPU_VPU] = {PWM_EF, MESON_PWM_0, VOLTAGE_TABLE_INFO(vdddos_npu_vpu_table)},
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
