/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PWM_H
#define __PWM_H

#ifdef __cplusplus
extern "C" {
#endif
#include <pwm_plat.h>

#define MESON_PWM_0 0
#define MESON_PWM_1 1
#define MESON_PWM_2 2
#define MESON_PWM_3 3

struct xPwmMesondevice {
	struct xPwmMesonChip *chip;
	uint32_t hwpwm;
	void *chip_data;
	uint32_t pwm_hi;
	uint32_t pwm_lo;
	uint32_t pwm_pre_div;
};

/**
 * vPwmMesonPwmDebug() - Dump pwm register
 * @pwm: pwm channel
 */
extern void vPwmMesonPwmDebug(struct xPwmMesondevice *pwm);

/**
 * vPwmMesonClear() - Clean pwm register
 * @pwm: pwm channel
 */
extern void vPwmMesonClear(struct xPwmMesondevice *pwm);

/**
 * vPwmMesonSetPolarity() - Set pwm polarity
 * @pwm: pwm channel
 * @val: pwm polarity
 */
extern void vPwmMesonSetPolarity(struct xPwmMesondevice *pwm, uint32_t val);

/**
 * xPwmMesonIsBlinkComplete() - Check blink complete
 * @pwm: pwm channel
 *
 * Returns 0 on blink complete.
 */
extern int32_t xPwmMesonIsBlinkComplete(struct xPwmMesondevice *pwm);

/**
 * prvPwmConstantDisable() - Disable constant function
 * @pwm: pwm channel
 *
 */
extern void vPwmConstantDisable(struct xPwmMesondevice *pwm);

/**
 * prvPwmConstantEnable() - Enable constant function
 * @pwm: pwm channel
 *
 */
extern void vPwmConstantEnable(struct xPwmMesondevice *pwm);

/**
 * vPwmMesonBlinkEnable() - Enable blink function
 * @pwm: pwm channel
 */
extern void vPwmMesonBlinkEnable(struct xPwmMesondevice *pwm);

/**
 * vPwmMesonBlinkDisable() - Disabled blink function
 * @pwm: pwm channel
 */
extern void vPwmMesonBlinkDisable(struct xPwmMesondevice *pwm);

/**
 * vPwmMesonSetBlinkTimes() - Set blink times
 * @pwm: pwm channel
 * @times: blink times
 */
extern void vPwmMesonSetBlinkTimes(struct xPwmMesondevice *pwm, uint32_t times);

/**
 * vPwmMesonSetTimes() - Set times
 * @pwm: pwm channel
 * @times: times
 */
extern void vPwmMesonSetTimes(struct xPwmMesondevice *pwm, uint32_t times);

/**
 * vPwmMesonEnable() - Enabled a pwm channel
 * @pwm: pwm channel
 */
extern void vPwmMesonEnable(struct xPwmMesondevice *pwm);

/**
 * vPwmMesonDisable() - Disabled a pwm channel
 * @pwm: pwm channel
 */
extern void vPwmMesonDisable(struct xPwmMesondevice *pwm);

/**
 * xPwmMesonConfig() - Config a pwm channel
 * @pwm: pwm channel
 * @duty_ns: effective time (ns)
 * @period_ns: period (ns)
 *
 * Returns 0 on success, negative value on error.
 */
extern int32_t xPwmMesonConfig(struct xPwmMesondevice *pwm, uint32_t duty_ns, uint32_t period_ns);

/**
 * vPwmMesonChannelFree() - Free a pwm channel
 * @pwm: pwm channel
 */
extern void vPwmMesonChannelFree(struct xPwmMesondevice *pwm);

/**
 * xPwmMesonChannelApply() - Apply a pwm channel
 * @chip_id: pwm controller id
 * @channel_id: pwm channel id
 *
 * Returns a pwm channel.
 */
extern struct xPwmMesondevice *xPwmMesonChannelApply(uint32_t chip_id, uint32_t channel_id);

/**
 * vPwmMesonsetvoltage() - Set voltage
 * @voltage_id: voltage select
 * @voltage_mv: voltage
 */
extern int32_t vPwmMesonsetvoltage(uint32_t voltage_id, uint32_t voltage_mv);

/**
 * vPwmMesongetvoltage() - Get voltage
 * @voltage_id: voltage select
 *
 * Returns voltage.
 */
extern int32_t vPwmMesongetvoltage(uint32_t voltage_id);

#ifdef __cplusplus
}
#endif
#endif
