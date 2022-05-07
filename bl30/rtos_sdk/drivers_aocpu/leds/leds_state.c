/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h" /* Must come first. */
#include "task.h" /* RTOS task related API prototypes. */
#include "queue.h" /* RTOS queue related API prototypes. */
#include "timers.h" /* Software timer related API prototypes. */
#include "semphr.h" /* Semaphore related API prototypes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>
#include <pwm.h>
#include <leds_plat.h>
#include <leds_state.h>
#include "mailbox-api.h"

/* todo: wait for stick mem ready */
static int32_t *PLedStickMem[LED_ID_MAX];

uint32_t LedMask[STICK_LED_INVAIL] = { STICK_LED_STATE_MASK,	  STICK_LED_BRIGHTNESS_MASK,
				       STICK_LED_BREATH_ID_MASK,  STICK_LED_BLINK_TIMES_MASK,
				       STICK_LED_BLINK_HIHG_MASK, STICK_LED_BLINK_LOW_MASK };

static int32_t prvStickMemLedRead(uint32_t id, enum StickMemLedIdx index, uint32_t *data)
{
	uint32_t temp;

	if ((index >= STICK_LED_INVAIL) || (id >= LED_ID_MAX)) {
		iprintf("%s: index: %d id: %ld read stick mem fail!\n", DRIVER_NAME, index, id);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	if (!PLedStickMem[id]) {
		iprintf("%s: id: %ld stick mem no ready!\n", DRIVER_NAME, id);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	temp = *PLedStickMem[id];
	*data = FIELD_GET(LedMask[index], temp);

	return 0;
}

static int32_t prvStickMemLedWrite(uint32_t id, enum StickMemLedIdx index, uint32_t data)
{
	uint32_t temp;

	if ((index >= STICK_LED_INVAIL) || (id >= LED_ID_MAX)) {
		iprintf("%s: index: %d id: %ld write stick mem fail!\n", DRIVER_NAME, index, id);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	if (!PLedStickMem[id]) {
		iprintf("%s: id: %ld stick mem no ready!\n", DRIVER_NAME, id);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	temp = *PLedStickMem[id];
	temp &= ~(LedMask[index]);
	temp |= FIELD_PREP(LedMask[index], data);
	*PLedStickMem[id] = temp;

	return 0;
}

static int32_t prvPwmledIsBlinkedComplete(uint32_t id)
{
	return xPwmMesonIsBlinkComplete(MesonLeds[id].pwm);
}

static int32_t prvPwmLedSetBrightness(uint32_t id, uint32_t brightness)
{
	uint32_t new_duty;
	unsigned long temp;

	if (MesonLeds[id].polarity) {
		/* TODO: Consider using hardware inversion later */
		//vPwmMesonSetPolarity(MesonLeds[id].pwm, LED_POLARITY_INVERT);
		brightness = LED_FULL - brightness;
	}
	temp = LED_PERIOD * (unsigned long)brightness;
	temp /= LED_FULL;
	new_duty = (uint32_t)temp;
	xPwmMesonConfig(MesonLeds[id].pwm, new_duty, LED_PERIOD);
	vPwmMesonDisable(MesonLeds[id].pwm_sub);
	vPwmMesonEnable(MesonLeds[id].pwm);

	return 0;
}

static int32_t prvPwmledblinktimes(uint32_t id, uint32_t times, uint32_t high_cnt, uint32_t low_cnt)
{
	uint32_t duty1, duty2;

	/* TODO: blink brightness */
	duty1 = 50000000; // 50ms
	duty2 = 0;

	if (MesonLeds[id].polarity) {
		/* TODO: Consider using hardware inversion later */
		//vPwmMesonSetPolarity(MesonLeds[id].pwm, LED_POLARITY_INVERT);
		xPwmMesonConfig(MesonLeds[id].pwm, duty1, duty1);
		xPwmMesonConfig(MesonLeds[id].pwm_sub, duty2, duty1);
	} else {
		xPwmMesonConfig(MesonLeds[id].pwm, duty2, duty1);
		xPwmMesonConfig(MesonLeds[id].pwm_sub, duty1, duty1);
	}
	vPwmConstantDisable(MesonLeds[id].pwm);
	vPwmMesonSetTimes(MesonLeds[id].pwm, high_cnt);
	vPwmMesonSetTimes(MesonLeds[id].pwm_sub, low_cnt);
	if (times) {
		vPwmMesonSetBlinkTimes(MesonLeds[id].pwm, times);
		vPwmMesonBlinkEnable(MesonLeds[id].pwm);
	} else {
		vPwmMesonBlinkDisable(MesonLeds[id].pwm);
	}
	vPwmMesonEnable(MesonLeds[id].pwm_sub);
	vPwmMesonEnable(MesonLeds[id].pwm);

	return 0;
}

static uint32_t prvCurveApproximate(struct LedCoord *c, uint32_t num, uint32_t x)
{
	uint32_t i, x0, y0, x1, y1, y = 0;

	for (i = 1; i <= num; i++) {
		if (x < c[i].x) {
			x0 = c[i - 1].x;
			y0 = c[i - 1].y;
			x1 = c[i].x;
			y1 = c[i].y;
			if (y1 == y0)
				y = y0;
			else if (y1 > y0)
				y = y0 + (y1 - y0) * (x - x0) / (x1 - x0);
			else
				y = y0 - (y0 - y1) * (x - x0) / (x1 - x0);
			break;
		}
	}
	return y;
}

static void prvLedBreath(uint32_t id, uint32_t breath_id)
{
	int32_t num = 0;
	uint32_t brightness;
	struct LedCoord *co;

	/*init breath_inflections parameters*/
	num = get_led_breath_len(breath_id);
	if (num <= 0) {
		iprintf("%s: id: %ld get breath len fail!\n", DRIVER_NAME, breath_id);
		return;
	}
	co = BreathInflections[breath_id];
	if (MesonLeds[id].breathtime > co[num - 1].x)
		MesonLeds[id].breathtime = 0;
	brightness = prvCurveApproximate(co, num, MesonLeds[id].breathtime);
	prvPwmLedSetBrightness(id, brightness);
	MesonLeds[id].breathtime += LED_TASK_TIME_MS;
}

static void prvSetLedStateToDefault(uint32_t id)
{
	prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_DEFAULT);
}

static uint32_t prvGetLedBlinkHigh(uint32_t id)
{
	uint32_t high;
	int32_t ret;

	ret = prvStickMemLedRead(id, STICK_LED_BLINK_HIHG, &high);
	if (ret) {
		iprintf("%s: id: %ld get led blink high fail return 1!\n", DRIVER_NAME, id);
		return LED_REEOR_CNT;
	}

	return high;
}

static uint32_t prvGetLedBlinkLow(uint32_t id)
{
	uint32_t low;
	int32_t ret;

	ret = prvStickMemLedRead(id, STICK_LED_BLINK_LOW, &low);
	if (ret) {
		iprintf("%s: id: %ld get led blink low fail return 1!\n", DRIVER_NAME, id);
		return LED_REEOR_CNT;
	}

	return low;
}

static uint32_t prvGetLedBlinkTimes(uint32_t id)
{
	uint32_t times;
	int32_t ret;

	ret = prvStickMemLedRead(id, STICK_LED_BLINK_TIMES, &times);
	if (ret) {
		iprintf("%s: id: %ld get led blink times fail return 1!\n", DRIVER_NAME, id);
		return LED_REEOR_CNT;
	}

	return times;
}

static uint32_t prvGetLedBreathId(uint32_t id)
{
	uint32_t breath_id;
	int32_t ret;

	ret = prvStickMemLedRead(id, STICK_LED_BREATH_ID, &breath_id);
	if (ret) {
		iprintf("%s: id: %ld get led breath id fail return 1!\n", DRIVER_NAME, id);
		return LED_REEOR_CNT;
	}

	return breath_id;
}

static uint32_t prvGetLedBrightness(uint32_t id)
{
	uint32_t brightness;
	int32_t ret;

	ret = prvStickMemLedRead(id, STICK_LED_BRIGHTNESS, &brightness);
	if (ret) {
		iprintf("%s: id: %ld get led brightness fail return 1!\n", DRIVER_NAME, id);
		return LED_REEOR_CNT;
	}

	return (brightness & 0xff);
}

static void prvSetLedState(enum LedState state, uint32_t id)
{
	if (state == MesonLeds[id].curr_led_state)
		return;
	MesonLeds[id].last_led_state = MesonLeds[id].curr_led_state;
	MesonLeds[id].curr_led_state = state;
}

static enum LedState prvGetLastLedState(uint32_t id)
{
	return MesonLeds[id].last_led_state;
}

static void prvLedStateMachine(enum LedState state, uint32_t id)
{
	static int blinkflag;

	prvSetLedState(state, id);

	switch (state) {
	/*
	 * led breath use the same timer task to handle breathe.
	 * so don't set the new state.
	 */
	case LED_STATE_BRIGHTNESS:
		prvPwmLedSetBrightness(id, prvGetLedBrightness(id));
		prvSetLedStateToDefault(id);
		blinkflag = 0;
		break;
	case LED_STATE_BREATH:
		prvLedBreath(id, prvGetLedBreathId(id));
		blinkflag = 0;
		break;
	case LED_STATE_BLINK_ON:
		if (blinkflag == 0) {
			prvPwmledblinktimes(id, prvGetLedBlinkTimes(id), prvGetLedBlinkHigh(id),
					    prvGetLedBlinkLow(id));
			prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BLINK_ON_HANDLE);
			blinkflag++;
		} else if (prvPwmledIsBlinkedComplete(id)) {
			prvPwmledblinktimes(id, prvGetLedBlinkTimes(id), prvGetLedBlinkHigh(id),
					    prvGetLedBlinkLow(id));
			prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BLINK_ON_HANDLE);
		}
		break;
	case LED_STATE_BLINK_OFF:
		if (blinkflag == 0) {
			prvPwmledblinktimes(id, prvGetLedBlinkTimes(id), prvGetLedBlinkHigh(id),
					    prvGetLedBlinkLow(id));
			prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BLINK_OFF_HANDLE);
			blinkflag++;
		} else if (prvPwmledIsBlinkedComplete(id)) {
			prvPwmledblinktimes(id, prvGetLedBlinkTimes(id), prvGetLedBlinkHigh(id),
					    prvGetLedBlinkLow(id));
			prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BLINK_OFF_HANDLE);
		}
		break;
	case LED_STATE_BLINK_BREATH:
		prvPwmledblinktimes(id, prvGetLedBlinkTimes(id), prvGetLedBlinkHigh(id),
				    prvGetLedBlinkLow(id));
		prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BLINK_BREATH_HANDLE);
		break;
	case LED_STATE_BLINK_ON_HANDLE:
		if ((prvPwmledIsBlinkedComplete(id) == 1) ||
		    (prvGetLastLedState(id) == LED_STATE_DEFAULT)) {
			prvPwmLedSetBrightness(id, LED_FULL);
			prvSetLedStateToDefault(id);
			blinkflag = 0;
		}
		break;
	case LED_STATE_BLINK_OFF_HANDLE:
		if ((prvPwmledIsBlinkedComplete(id) == 1) ||
		    (prvGetLastLedState(id) == LED_STATE_DEFAULT)) {
			prvPwmLedSetBrightness(id, LED_OFF);
			prvSetLedStateToDefault(id);
			blinkflag = 0;
		}
		break;
	case LED_STATE_BLINK_BREATH_HANDLE:
		if ((prvPwmledIsBlinkedComplete(id) == 1) ||
		    (prvGetLastLedState(id) == LED_STATE_DEFAULT)) {
			prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BREATH);
		}
		break;
	case LED_STATE_INVALID:
		break;
	case LED_STATE_DEFAULT:
		break;
	} /* end swith */
}

static void *prvLedGetInfo(void *msg)
{
	uint32_t state, id, brightness, breath, blink_times, blink_high, blink_low;

	id = *(u32 *)msg;
	state = *(((u32 *)msg) + 1);
	switch (state) {
	/*
	 * led breath use the same timer task to handle breathe.
	 * so don't set the new state.
	 */
	case LED_STATE_BRIGHTNESS:
		brightness = *(((u32 *)msg) + 2);
		xLedsStateSetBrightness(id, brightness);
		break;
	case LED_STATE_BREATH:
		breath = *(((u32 *)msg) + 2);
		xLedsStateSetBreath(id, breath);
		break;
	case LED_STATE_BLINK_ON:
		blink_times = *(((u32 *)msg) + 2);
		blink_high = *(((u32 *)msg) + 3);
		blink_low = *(((u32 *)msg) + 4);
		xLedsStateSetBlinkOn(id, blink_times, blink_high, blink_low, 0, 0);
		break;
	case LED_STATE_BLINK_OFF:
		blink_times = *(((u32 *)msg) + 2);
		blink_high = *(((u32 *)msg) + 3);
		blink_low = *(((u32 *)msg) + 4);
		xLedsStateSetBlinkOff(id, blink_times, blink_high, blink_low, 0, 0);
		break;
	case LED_STATE_BLINK_BREATH:
		blink_times = *(((u32 *)msg) + 2);
		blink_high = *(((u32 *)msg) + 3);
		blink_low = *(((u32 *)msg) + 4);
		xLedsStateSetBlinkBreath(id, blink_times, blink_high, blink_low, 0, 0);
		break;
	case LED_STATE_INVALID:
		break;
	case LED_STATE_DEFAULT:
		break;
	} /* end switth */

	return NULL;
}

static void vPrintLedsStatus(TimerHandle_t xTimer)
{
	uint32_t i, state = 0;

	(void)xTimer;
	//taskENTER_CRITICAL();
	for (i = 0; i < LED_ID_MAX; i++) {
		prvStickMemLedRead(i, STICK_LED_STATE, &state);
		prvLedStateMachine(state, i);
	}
	//taskEXIT_CRITICAL();
}

int32_t xLedsStateSetBlinkBreath(uint32_t id, uint32_t times, uint32_t high_ms, uint32_t low_ms,
				 uint32_t high_br, uint32_t low_br)
{
	if ((id >= LED_ID_MAX) || (times > LED_MAX_BLINK_CNT)) {
		iprintf("%s: id: %ld times: %ld set blink breath fail! maxid: %d maxtime: %d\n",
			DRIVER_NAME, id, times, LED_ID_MAX - 1, LED_MAX_BLINK_CNT);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	if ((high_ms > LED_MAX_HIGH_MS) || (low_ms > LED_MAX_HIGH_MS)) {
		iprintf("%s: high: %ldms low: %ldms set blink breath fail!
			maxhigh: %dms maxlow: %dms\n",
			DRIVER_NAME, high_ms, low_ms, LED_MAX_HIGH_MS, LED_MAX_LOW_MS);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	/* TODO: high_br low_br no ready */
	if (low_br || high_br)
		iprintf("%s: low_br high_br no ready!\n", DRIVER_NAME);

	prvStickMemLedWrite(id, STICK_LED_BLINK_TIMES, times);
	prvStickMemLedWrite(id, STICK_LED_BLINK_HIHG, high_ms / 50);
	prvStickMemLedWrite(id, STICK_LED_BLINK_LOW, low_ms / 50);
	prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BLINK_BREATH);

	return 0;
}

int32_t xLedsStateSetBlinkOn(uint32_t id, uint32_t times, uint32_t high_ms, uint32_t low_ms,
			     uint32_t high_br, uint32_t low_br)
{
	if ((id >= LED_ID_MAX) || (times > LED_MAX_BLINK_CNT)) {
		iprintf("%s: id: %ld times: %ld set blink on fail! maxid: %d maxtime: %d\n",
			DRIVER_NAME, id, times, LED_ID_MAX - 1, LED_MAX_BLINK_CNT);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	if ((high_ms > LED_MAX_HIGH_MS) || (low_ms > LED_MAX_HIGH_MS)) {
		iprintf("%s: high: %ldms low: %ldms set blink on fail!
			maxhigh: %dms maxlow: %dms\n",
			DRIVER_NAME, high_ms, low_ms, LED_MAX_HIGH_MS, LED_MAX_LOW_MS);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	/* TODO: high_br low_br no ready */
	if (low_br || high_br)
		iprintf("%s: low_br high_br no ready!\n", DRIVER_NAME);

	prvStickMemLedWrite(id, STICK_LED_BLINK_TIMES, times);
	prvStickMemLedWrite(id, STICK_LED_BLINK_HIHG, high_ms / 50);
	prvStickMemLedWrite(id, STICK_LED_BLINK_LOW, low_ms / 50);
	prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BLINK_ON);

	return 0;
}

int32_t xLedsStateSetBlinkOff(uint32_t id, uint32_t times, uint32_t high_ms, uint32_t low_ms,
			      uint32_t high_br, uint32_t low_br)
{
	if ((id >= LED_ID_MAX) || (times > LED_MAX_BLINK_CNT)) {
		iprintf("%s: id: %ld times: %ld set blink off fail! maxid: %d maxtime: %d\n",
			DRIVER_NAME, id, times, LED_ID_MAX - 1, LED_MAX_BLINK_CNT);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	if ((high_ms > LED_MAX_HIGH_MS) || (low_ms > LED_MAX_HIGH_MS)) {
		iprintf("%s: high: %ldms low: %ldms set blink off fail!
			maxhigh: %dms maxlow: %dms\n",
			DRIVER_NAME, high_ms, low_ms, LED_MAX_HIGH_MS, LED_MAX_LOW_MS);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	/* TODO: high_br low_br no ready */
	if (low_br || high_br)
		iprintf("%s: low_br high_br no ready!\n", DRIVER_NAME);

	prvStickMemLedWrite(id, STICK_LED_BLINK_TIMES, times);
	prvStickMemLedWrite(id, STICK_LED_BLINK_HIHG, high_ms / 50);
	prvStickMemLedWrite(id, STICK_LED_BLINK_LOW, low_ms / 50);
	prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BLINK_OFF);

	return 0;
}

int32_t xLedsStateSetBreath(uint32_t id, uint32_t breath_id)
{
	if ((id >= LED_ID_MAX) || (breath_id >= LED_BREATH_MAX_COUNT)) {
		iprintf("%s: id: %ld breath id: %ld set breath fail! maxid: %d maxbreath id: %d\n",
			DRIVER_NAME, id, breath_id, LED_ID_MAX - 1, LED_BREATH_MAX_COUNT - 1);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	MesonLeds[id].breathtime = 0;
	prvStickMemLedWrite(id, STICK_LED_BREATH_ID, breath_id);
	prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BREATH);

	return 0;
}

int32_t xLedsStateSetBrightness(uint32_t id, uint32_t brightness)
{
	if ((id >= LED_ID_MAX) || (brightness > LED_FULL)) {
		iprintf("%s: id: %ld brightness: %ld set brightness fail!
			maxid: %d maxbrightness: %d\n",
			DRIVER_NAME, id, brightness, LED_ID_MAX - 1, LED_FULL);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	prvStickMemLedWrite(id, STICK_LED_BRIGHTNESS, brightness);
	prvStickMemLedWrite(id, STICK_LED_STATE, LED_STATE_BRIGHTNESS);

	return 0;
}

int32_t xLedsStateInit(void)
{
	TimerHandle_t xLedsTimer = NULL;
	int32_t ret, i;

	iprintf("%s: leds state init!\n", DRIVER_NAME);
	/* TODO: free */
	/* apply pwm */
	for (i = 0; i < LED_ID_MAX; i++) {
		MesonLeds[i].pwm =
			xPwmMesonChannelApply(MesonLeds[i].hardware_id / LED_PWM_CHAN_CNT,
					      MesonLeds[i].hardware_id % LED_PWM_CHAN_CNT);
		if (!MesonLeds[i].pwm) {
			iprintf("%s: id: %ld chip: %ld channel: %ld apply pwm channel fail!\n",
				DRIVER_NAME, i, MesonLeds[i].hardware_id / LED_PWM_CHAN_CNT,
				MesonLeds[i].hardware_id % LED_PWM_CHAN_CNT);
			return -pdFREERTOS_ERRNO_EINVAL;
		}
		MesonLeds[i].pwm_sub = xPwmMesonChannelApply(
			MesonLeds[i].hardware_id / LED_PWM_CHAN_CNT,
			MesonLeds[i].hardware_id % LED_PWM_CHAN_CNT + LED_PWM_CHAN_CNT);
		if (!MesonLeds[i].pwm_sub) {
			iprintf("%s: id: %ld chip: %ld channel: %ld apply pwm channel fail!\n",
				DRIVER_NAME, i, MesonLeds[i].hardware_id / LED_PWM_CHAN_CNT,
				MesonLeds[i].hardware_id % LED_PWM_CHAN_CNT + LED_PWM_CHAN_CNT);
			return -pdFREERTOS_ERRNO_EINVAL;
		}
	}

	/* register mailbox */
	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_GET_LED_INFO,
						    prvLedGetInfo, 1);
	if (ret == MBOX_CALL_MAX) {
		iprintf("%s: mbox cmd 0x%x register fail\n", DRIVER_NAME, MBX_CMD_GET_LED_INFO);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	/* initialization status */
	vLedPlatInit(PLedStickMem);
	/* creat led timer */
	xLedsTimer = xTimerCreate("Timer", pdMS_TO_TICKS(LED_TASK_TIME_MS), pdTRUE, NULL,
				  vPrintLedsStatus);
	iprintf("%s: Starting leds task ...\n", DRIVER_NAME);
	xTimerStart(xLedsTimer, 0);

	/* pinmux needs setting at the end */
	return vLedPinmuxInit();
}
