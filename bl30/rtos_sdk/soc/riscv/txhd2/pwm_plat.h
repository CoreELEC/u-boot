/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _MESON_PWM_PLAT_H_
#define _MESON_PWM_PLAT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <register.h>

enum meson_pwm_id {
	MESON_PWM_A = 0,
	MESON_PWM_B,
	MESON_PWM_C,
	MESON_PWM_D,
	MESON_PWM_E,
	MESON_PWM_F,
	MESON_PWM_AO_A,
	MESON_PWM_AO_B,
	MESON_PWM_AO_C,
	MESON_PWM_AO_D,
	MESON_PWM_INVALID,
};

#define PwmMesonVolt_Duty 1

/* There are 3 pwm controllers in t5d */
enum pwm_chip_id {
	PWM_AB = 0,
	PWM_CD,
	PWM_EF,
	PWM_AO_AB,
	PWM_AO_CD,
	PWM_MUX,
};

/* VDDEE VDDCPU in t5d */
enum pwm_voltage_id {
	VDDEE_VOLT = 0,
	VDDCPU_VOLT,
};

struct xPwmMesonVoltage {
	uint32_t Voltage_mv;
	uint32_t Duty_reg;
};

uint32_t prvMesonVoltToPwmchip(enum pwm_voltage_id voltage_id);
uint32_t prvMesonVoltToPwmchannel(enum pwm_voltage_id voltage_id);
struct xPwmMesonVoltage *vPwmMesonGetVoltTable(uint32_t voltage_id);
uint32_t vPwmMesonGetVoltTableSize(uint32_t voltage_id);
struct xPwmMesonChip *prvIdToPwmChip(uint32_t chip_id);

#ifdef __cplusplus
}
#endif
#endif /* _MESON_PWM_PLAT_H_ */
