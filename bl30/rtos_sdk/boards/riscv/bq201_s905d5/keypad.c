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
#include "gpio.h"
#include "suspend.h"

/* KEY ID */
#define GPIO_KEY_ID_POWER GPIOD_3
#define GPIO_ETH_WOL

static void vGpioKeyCallBack(struct xReportEvent event)
{
	uint32_t buf[4] = { 0 };

	switch (event.ulCode) {
	case GPIO_KEY_ID_POWER:
		buf[0] = POWER_KEY_WAKEUP;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		break;
	case GPIOZ_14:
		printf("gpio14 wakeup\n");
		buf[0] = ETH_PHY_GPIO;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		break;
	default:
		break;
	}

	printf("GPIO key event 0x%x, key code %d, responseTicks %d\n", event.event, event.ulCode,
	       event.responseTime);
}

struct xGpioKeyInfo gpioKeyInfo[] = {
	GPIO_KEY_INFO(GPIO_KEY_ID_POWER, HIGH, EVENT_SHORT, vGpioKeyCallBack, NULL),
#ifdef GPIO_ETH_WOL
	GPIO_KEY_INFO(GPIOZ_14, HIGH, EVENT_SHORT, vGpioKeyCallBack, NULL),
#endif
};

void vKeyPadInit(void)
{
	vCreateGpioKey(gpioKeyInfo, sizeof(gpioKeyInfo) / sizeof(struct xGpioKeyInfo));
	vGpioKeyEnable();
}

void vKeyPadDeinit(void)
{
	vGpioKeyDisable();
	vDestroyGpioKey();
}
