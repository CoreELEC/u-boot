/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MESON_KEYPAD_H
#define __MESON_KEYPAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include <timers.h>
#include <task.h>
#include <gpio.h>
#include <saradc.h>

/* common macro */
#define TIMER_CYCLE_TIME	100
#define KEY_JITTER_COUNT	1

/* key event */
#define EVENT_SHORT		(1 << 0)
#define EVENT_LONG		(1 << 1)

/* adc keypad */
#define ADCKEY_ID_BASE		512
#define SAMPLE_DEVIATION	160

enum KeyType { GPIO_KEY = 0, ADC_KEY, KEY_TYPE_NUM };

enum GpioLevel { LOW = 0, HIGH };

enum KeyState {
	UP = 0,
	DOWN,
};

struct xReportEvent {
	uint32_t ulCode;
	uint32_t event;
	TickType_t responseTime;
	void *data;
};

typedef void (*CallBack_t)(struct xReportEvent arg);

struct xKeyInitInfo {
	uint32_t ulKeyId;
	uint32_t eventMask;
	uint32_t repeatTimeMs;
	uint32_t repeatDelayTimeMs;
	uint32_t longDTTMs;
	uint32_t doubleDTTMs;
	uint32_t combLongDTTMs;
	uint32_t combDTTMs;
	CallBack_t CallBack;
	void *data;
};

struct xGpioKeyInfo {
	int ulInitLevel;
	struct xKeyInitInfo keyInitInfo;
};

struct xAdcKeyInfo {
	int32_t ulValue;
	struct AdcInstanceConfig xAdcDecp;
	struct xKeyInitInfo keyInitInfo;
};

#define KEY_INIT_INFO(_ulKeyId, _eventMask, _repeatTimeMs, _repeatDelayTimeMs, _longDTTMs,         \
		      _doubleDTTMs, _combLongDTTMs, _combDTTMs, _CallBack, _data)                  \
	{                                                                                          \
		.ulKeyId = _ulKeyId, .eventMask = _eventMask, .repeatTimeMs = _repeatTimeMs,       \
		.repeatDelayTimeMs = _repeatDelayTimeMs, .longDTTMs = _longDTTMs,                  \
		.doubleDTTMs = _doubleDTTMs, .combLongDTTMs = _combLongDTTMs,                      \
		.combDTTMs = _combDTTMs, .CallBack = _CallBack, .data = _data,                     \
	}

#define GPIO_KEY_INFO(_ulKeyId, _ulInitLevel, _eventMask, _CallBack, _data)                        \
	{                                                                                          \
		.ulInitLevel = _ulInitLevel,                                                       \
		.keyInitInfo =                                                                     \
			KEY_INIT_INFO(_ulKeyId, _eventMask, 0, 0, 0, 0, 0, 0, _CallBack, _data),   \
	}

#define ADC_KEY_INFO(_ulKeyId, _ulValue, _adcChan, _eventMask, _CallBack, _data)                   \
	{                                                                                          \
		.ulValue = _ulValue, .xAdcDecp = { _adcChan, NO_AVERAGING, 1 },                    \
		.keyInitInfo =                                                                     \
			KEY_INIT_INFO(_ulKeyId, _eventMask, 0, 0, 0, 0, 0, 0, _CallBack, _data),   \
	}

void vCreateGpioKey(struct xGpioKeyInfo *keyArr, uint16_t keyNum);
void vDestroyGpioKey(void);
void vGpioKeyEnable(void);
void vGpioKeyDisable(void);
int vGpioKeyIsEmpty(void);

void vCreateAdcKey(struct xAdcKeyInfo *keyArr, uint16_t keyNum);
void vDestroyAdcKey(void);
void vAdcKeyEnable(void);
void vAdcKeyDisable(void);
int vAdcKeyIsEmpty(void);

void vKeyPadCreate(void);
void vKeyPadInit(void);
void vKeyPadDeinit(void);

void vDynamicKeypadInit(void);

#ifdef __cplusplus
}
#endif

#endif /* __MESON_KEYPAD_H */

