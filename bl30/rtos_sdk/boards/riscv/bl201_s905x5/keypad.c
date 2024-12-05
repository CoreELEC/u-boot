/*
 * Copyright (c) 2021-2024 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "soc.h"
#include "keypad.h"
#include "saradc.h"
#include "suspend.h"

/* KEY ID */
#define ADC_KEY_ID_POWER 520
#define GPIO_ETH_WOL

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

	printf("ADCKEY: EVENT 0x%x,%d,%d\n", event.event, event.ulCode,
	       event.responseTime);
}

static void vGpioKeyCallBack(struct xReportEvent event)
{
	uint32_t buf[4] = { 0 };

	switch (event.ulCode) {
	case GPIOZ_14:
		printf("gpio14 wakeup\n");
		buf[0] = ETH_PHY_GPIO;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		break;
	default:
		break;
	}

	printf("GPIOKEY: EVENT 0x%x,%d,%d\n", event.event, event.ulCode,
	       event.responseTime);
}

struct xGpioKeyInfo gpioKeyInfo[] = {
#ifdef GPIO_ETH_WOL
	GPIO_KEY_INFO(GPIOZ_14, HIGH, EVENT_SHORT, vGpioKeyCallBack, NULL),
#endif
};
struct xAdcKeyInfo adcKeyInfo[] = {
	ADC_KEY_INFO(ADC_KEY_ID_POWER, 90, SARADC_CH3, EVENT_SHORT, vAdcKeyCallBack, NULL),
};

void vKeyPadInit(void)
{
	vCreateAdcKey(adcKeyInfo, sizeof(adcKeyInfo) / sizeof(struct xAdcKeyInfo));
	vAdcKeyEnable();
	vCreateGpioKey(gpioKeyInfo, sizeof(gpioKeyInfo) / sizeof(struct xGpioKeyInfo));
	vGpioKeyEnable();
}

void vKeyPadDeinit(void)
{
	vAdcKeyDisable();
	vDestroyAdcKey();
	vGpioKeyDisable();
	vDestroyGpioKey();
}
