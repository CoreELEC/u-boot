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
#include "suspend.h"

static void vGpioKeyCallBack(struct xReportEvent event)
{
	uint32_t buf[4] = { 0 };

	if (event.ulCode == get_User_Gpio()) {
		buf[0] = POWER_KEY_WAKEUP;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
	}

	printf("GPIOKEY: EVENT 0x%x,%d,%d\n", event.event, event.ulCode,
	       event.responseTime);
}

struct xGpioKeyInfo gpioGpioKeyInfo[] = {
	GPIO_KEY_INFO(GPIO_INVALID, HIGH, EVENT_SHORT,
			vGpioKeyCallBack, NULL)
};

void vKeyPadInit(void)
{
	gpioGpioKeyInfo[0].keyInitInfo.ulKeyId = get_User_Gpio();
	vCreateGpioKey(gpioGpioKeyInfo, sizeof(gpioGpioKeyInfo) / sizeof(struct xGpioKeyInfo));
	vGpioKeyEnable();
}

void vKeyPadDeinit(void)
{
	vGpioKeyDisable();
	vDestroyGpioKey();
}
