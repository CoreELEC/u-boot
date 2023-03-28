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
#include "vad_suspend.h"

/*KEY ID*/
#define GPIO_KEY_ID_POWER GPIOD_3
#define GPIO_KEY_ID_WIFI_WAKE GPIOD_12

#define ADC_KEY_ID_MENU 520

static void vGpioKeyCallBack(struct xReportEvent event)
{
	uint32_t buf[4] = { 0 };
#ifdef KEYPAD_USED
	switch (event.ulCode) {
#ifdef CONFIG_WIFI_WAKEUP
	case GPIO_KEY_ID_WIFI_WAKE:
		buf[0] = WIFI_WAKEUP;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		wakeup_dsp();
		break;
#endif
	case GPIO_KEY_ID_POWER:
		buf[1] = POWER_KEY_WAKEUP;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		wakeup_dsp();
		break;
	default:
		break;
	}
#endif
	printf("GPIO key event 0x%x, key code %d, responseTicks %d\n", event.event, event.ulCode,
	       event.responseTime);
}

static void vAdcKeyCallBack(struct xReportEvent event)
{
	uint32_t buf[4] = { 0 };

	switch (event.ulCode) {
	case ADC_KEY_ID_MENU:
		buf[0] = POWER_KEY_WAKEUP;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		wakeup_dsp();
		break;
	default:
		break;
	}

	printf("ADC key event 0x%x, key code %d, responseTime %d\n", event.event, event.ulCode,
	       event.responseTime);
}

struct xGpioKeyInfo gpioKeyInfo[] = {
	/*
	 * GPIO_KEY_INFO(GPIO_KEY_ID_POWER, HIGH, EVENT_SHORT,
	 *		vGpioKeyCallBack, NULL),
	 */
#ifdef CONFIG_WIFI_WKAEUP
	GPIO_KEY_INFO(GPIO_KEY_ID_WIFI_WAKE, HIGH, EVENT_SHORT, vGpioKeyCallBack, NULL)
#endif

};

struct xAdcKeyInfo adcKeyInfo[] = {
	ADC_KEY_INFO(ADC_KEY_ID_MENU, 0, SARADC_CH1, EVENT_SHORT, vAdcKeyCallBack, NULL),
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
