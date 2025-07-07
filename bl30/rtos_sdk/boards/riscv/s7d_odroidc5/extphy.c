/*
 * Copyright (c) 2025 Hardkernel Co., Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include "keypad.h"
#include "gpio.h"
#include "suspend.h"

#define GPIO_EXTPHY_WOL		GPIOH_10

static void vExtPhyCallBack(struct xReportEvent event)
{
	uint32_t buf[4] = { 0 };

	if (event.ulCode == GPIO_EXTPHY_WOL) {
		buf[0] = ETH_PHY_GPIO;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		printf("EXTPHY: EVENT 0x%x,%d,%d\n", event.event, event.ulCode,
				event.responseTime);
	}
}

struct xGpioKeyInfo gpioKeyInfo[] = {
	GPIO_KEY_INFO(GPIO_EXTPHY_WOL, HIGH, EVENT_SHORT, vExtPhyCallBack, NULL),
};

void vExtPhyInit(void)
{
	if (get_ETHWol_flag())
		vCreateGpioKey(gpioKeyInfo, sizeof(gpioKeyInfo) / sizeof(struct xGpioKeyInfo));
}

void vExtPhyDeinit(void)
{
	vDestroyGpioKey();
}
