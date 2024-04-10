/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "ir.h"
#include "soc.h"
#include "keypad.h"
#include "gpio.h"
#include "saradc.h"
#include "suspend.h"

/*KEY ID*/
#define ADC_KEY_ID_POWER 520

static void vAdcKeyCallBack(struct xReportEvent event)
{
	uint32_t buf[4] = { 0 };

	switch (event.ulCode) {
	case ADC_KEY_ID_POWER:
		buf[0] = POWER_KEY_WAKEUP;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		break;
	default:
		break;
	}

	printf("ADC key event 0x%x, key code %d, responseTime %d\n", event.event, event.ulCode,
	       event.responseTime);
}

struct xAdcKeyInfo adcKeyInfo[] = {
	ADC_KEY_INFO(ADC_KEY_ID_POWER, 90, SARADC_CH0, EVENT_SHORT, vAdcKeyCallBack, NULL),
};

void vKeyPadInit(void)
{
	vCreateAdcKey(adcKeyInfo, sizeof(adcKeyInfo) / sizeof(struct xAdcKeyInfo));
	vAdcKeyEnable();
}

void vKeyPadDeinit(void)
{
	vAdcKeyDisable();
	vDestroyAdcKey();
}
