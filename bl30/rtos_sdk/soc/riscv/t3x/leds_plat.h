/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * platform related header file
 */

#ifndef _MESON_LEDS_PLAT_H_
#define _MESON_LEDS_PLAT_H_

#ifdef __cplusplus
extern "C" {
#endif
/*
 * pwm S4 leds config
 */
#include <leds_state.h>

#define LED_BREATH_MAX_COUNT 4

extern struct LedDevice MesonLeds[];

enum led_pwm_id {
	LED_PWM_A = 0,
	LED_PWM_B,
	LED_PWM_C,
	LED_PWM_D,
	LED_PWM_E,
	LED_PWM_F,
	LED_PWM_G,
	LED_PWM_H,
	LED_PWM_I,
	LED_PWM_J,
	LED_PWM_INVALID,
};

enum led_id {
	LED_ID_0 = 0,
	LED_ID_MAX,
};

int32_t get_led_breath_len(uint32_t breath_id);
int32_t vLedPinmuxInit(void);
int32_t vLedPlatInit(int32_t **stickmem);

#ifdef __cplusplus
}
#endif
#endif /* _MESON_PWM_PLAT_H_ */
