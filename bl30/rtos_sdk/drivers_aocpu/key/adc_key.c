/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <timers.h>
#include <task.h>
#include "keypad.h"
#include "saradc.h"

struct xOneAdcKeyInfo {
	enum KeyState keyState;
	struct xAdcKeyInfo *adcKeyInfo;
	struct xOneAdcKeyInfo *xNext;
};

static int jitterCount;
static struct xOneAdcKeyInfo *xHeadKey;
static TimerHandle_t xAdcKeyCycleTimer;

static void prReportEvent(struct xAdcKeyInfo *xKey, uint32_t event)
{
	struct xReportEvent reportEvent;

	reportEvent.ulCode = xKey->keyInitInfo.ulKeyId;
	reportEvent.data = xKey->keyInitInfo.data;
	reportEvent.event = event;
	reportEvent.responseTime = 0;

	if (xKey->keyInitInfo.CallBack)
		xKey->keyInitInfo.CallBack(reportEvent);
}

static void prAdcKeyProcess(TimerHandle_t xTimer)
{
	int ret;
	uint16_t usAdcData;
	struct xOneAdcKeyInfo *xPassBtn = xHeadKey;
	struct xAdcKeyInfo *adcKeyInfo;

	(void)xTimer;
	for (xPassBtn = xHeadKey; xPassBtn != NULL; xPassBtn = xPassBtn->xNext) {
		adcKeyInfo = xPassBtn->adcKeyInfo;
		ret = xAdcGetSample(&usAdcData, 1, &(adcKeyInfo->xAdcDecp));
		if (ret < 0)
			continue;
		if (xPassBtn->keyState == UP) {
			if ((usAdcData >= adcKeyInfo->ulValue - SAMPLE_DEVIATION) &&
			    (usAdcData <= adcKeyInfo->ulValue + SAMPLE_DEVIATION)) {
				if (jitterCount < KEY_JITTER_COUNT) {
					jitterCount++;
					return;
				}
				jitterCount = 0;
				xPassBtn->keyState = DOWN;
				prReportEvent(xPassBtn->adcKeyInfo, EVENT_SHORT);
			}

		} else if (xPassBtn->keyState == DOWN) {
			if (((usAdcData <= adcKeyInfo->ulValue - SAMPLE_DEVIATION) ||
			     (usAdcData >= adcKeyInfo->ulValue + SAMPLE_DEVIATION))) {
				if (jitterCount < KEY_JITTER_COUNT) {
					jitterCount++;
					return;
				}
				jitterCount = 0;
				xPassBtn->keyState = UP;
			}
		}
	}
}

static void prvAddAdcKey(struct xOneAdcKeyInfo *xKey)
{
	if (!xKey)
		return;
	xKey->xNext = xHeadKey;
	xHeadKey = xKey;
}

void vCreateAdcKey(struct xAdcKeyInfo *keyArr, uint16_t keyNum)
{
	uint16_t i;
	struct xOneAdcKeyInfo *xOneKey;
	struct xKeyInitInfo *keyInitInfo;
	struct xAdcKeyInfo *adcKeyInfo;

	if (!xAdcKeyCycleTimer) {
		xAdcKeyCycleTimer =
			xTimerCreate((const char *)"xAdcKeyTimer", pdMS_TO_TICKS(TIMER_CYCLE_TIME),
				     pdTRUE, (void *)1, (TimerCallbackFunction_t)prAdcKeyProcess);
		if (!xAdcKeyCycleTimer) {
			printf("adc timer create failed!\n");
			return;
		}
	}

	for (i = 0; i < keyNum; i++) {
		keyInitInfo = &(keyArr[i].keyInitInfo);

		if (keyInitInfo->ulKeyId < ADCKEY_ID_BASE) {
			printf("adc key:[%d] key code not less than 512.\n", i);
			continue;
		}

		xOneKey = pvPortMalloc(sizeof(struct xOneAdcKeyInfo));
		if (xOneKey == NULL)
			goto fail_alloc2;

		adcKeyInfo = pvPortMalloc(sizeof(struct xAdcKeyInfo));
		if (adcKeyInfo == NULL)
			goto fail_alloc1;

		memcpy(adcKeyInfo, &keyArr[i], sizeof(struct xAdcKeyInfo));
		memset(xOneKey, 0, sizeof(struct xOneAdcKeyInfo));
		xOneKey->keyState = UP;
		xOneKey->adcKeyInfo = adcKeyInfo;
		prvAddAdcKey(xOneKey);

		printf("keypad: add adc key [%ld]\n", keyInitInfo->ulKeyId);
	}

	return;

fail_alloc1:
	vPortFree(xOneKey);
fail_alloc2:
	printf("adc key: [%d] malloc failed!\n", i);
}

void vDestoryAdcKey(void)
{
	struct xOneAdcKeyInfo *xPassBtn, *xTmpBtn;
	uint32_t key_id;

	for (xPassBtn = xHeadKey; xPassBtn != NULL;) {
		key_id = xPassBtn->adcKeyInfo->keyInitInfo.ulKeyId;

		vPortFree(xPassBtn->adcKeyInfo);

		xTmpBtn = xPassBtn;
		xPassBtn = xPassBtn->xNext;

		vPortFree(xTmpBtn);

		printf("keypad: del adc key [%ld]\n", key_id);
	}

	xHeadKey = NULL;
}

void vAdcKeyEnable(void)
{
	vAdcInit();
	vAdcHwEnable();

	if (xAdcKeyCycleTimer)
		xTimerStart(xAdcKeyCycleTimer, 0);
}

void vAdcKeyDisable(void)
{
	if (xAdcKeyCycleTimer)
		xTimerStop(xAdcKeyCycleTimer, pdMS_TO_TICKS(TIMER_CYCLE_TIME));

	vAdcHwDisable();
	vAdcDeinit();
}

int vAdcKeyIsEmpty(void)
{
	return (!xHeadKey);
}
