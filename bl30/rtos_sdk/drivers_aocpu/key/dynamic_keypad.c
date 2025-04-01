/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "keypad.h"
#include "gpio.h"
#include "saradc.h"
#include "suspend.h"
#include "mailbox-api.h"

/*KEY ID*/
#define GPIO_KEY_ID_POWER GPIOD_3

#define ADC_KEY_ID_HOME 520

static void vKeyCallBack(struct xReportEvent event)
{
	uint32_t buf[4] = { 0 };

	buf[0] = POWER_KEY_WAKEUP;
	STR_Wakeup_src_Queue_Send(buf);

	printf("DYNKEY: EVENT 0x%x,%d,%d\n", event.event, event.ulCode, event.responseTime);
}

static void *xMboxSetKeypad(void *msg)
{
	uint32_t *key_info = (uint32_t *)msg;

	if (key_info[0] == 0xFFFFFFFF) {
		/* remove all key */
		vDestroyAdcKey();
		vDestroyGpioKey();

		return NULL;
	}

	if ((key_info[3] < 1) || (key_info[3] > 2)) {
		printf("DYNKEY: INVAL PARAM\n");
		return NULL;
	}

	if (key_info[0] >= ADCKEY_ID_BASE) {
		if (key_info[1] > ((1 << 12) - 1)) {
			printf("DYNKEY: INVAL ADC VAL\n");
			return NULL;
		}

		if (key_info[2] > 7) {
			printf("DYNKEY: INVAL ADC CHAN\n");
			return NULL;
		}

		struct xAdcKeyInfo adcKey[1] = { ADC_KEY_INFO(key_info[0], key_info[1],
							      (enum AdcChannelType)key_info[2],
							      key_info[3], vKeyCallBack, NULL) };

		vCreateAdcKey(adcKey, 1);
	} else if (key_info[0] < ADCKEY_ID_BASE) {
		if (key_info[1] > 1) {
			printf("DYNKEY: INVAL LEVEL\n");
			return NULL;
		}

		struct xGpioKeyInfo gpioKey[1] = { GPIO_KEY_INFO(key_info[0], key_info[1],
								 key_info[2], vKeyCallBack, NULL) };

		vCreateGpioKey(gpioKey, 1);
	}

	return NULL;
}

void vDynamicKeypadInit(void)
{
	int ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_SET_KEYPAD, xMboxSetKeypad,
						    0);
	if (ret)
		printf("DYNKEY: MBOX SETUP FAIL %x\n", CMD_SET_KEYPAD);
}
