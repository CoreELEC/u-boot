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
 * pwm a5 leds config
 */
#include <leds_state.h>
#include <pwm_plat.h>

#define LED_BREATH_MAX_COUNT 4

extern struct LedDevice MesonLeds[];

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
#endif /* _MESON_LED_PLAT_H_ */
