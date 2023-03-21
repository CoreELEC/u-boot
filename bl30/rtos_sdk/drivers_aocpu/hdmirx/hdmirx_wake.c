/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include "FreeRTOS.h"
#include "suspend.h"
#include "task.h"
#include "gpio.h"

#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include "hdmirx_wake.h"

#define GPIO_HDMI_RX1_POWER	GPIOW_1
#define GPIO_HDMI_RX2_POWER	GPIOW_9
#define GPIO_HDMI_RX3_POWER	GPIOW_5
#define GPIO_HDMI_RX4_POWER	GPIOW_13


static void hdmirx_IRQHandle(void)
{
	uint32_t buf[4] = {0};

	vDisableGpioIRQ(GPIO_HDMI_RX1_POWER);
	vDisableGpioIRQ(GPIO_HDMI_RX2_POWER);
	vDisableGpioIRQ(GPIO_HDMI_RX3_POWER);
	vDisableGpioIRQ(GPIO_HDMI_RX4_POWER);
	if (xGpioGetValue(GPIO_HDMI_RX1_POWER) ||
		xGpioGetValue(GPIO_HDMI_RX2_POWER) ||
		xGpioGetValue(GPIO_HDMI_RX3_POWER) ||
		xGpioGetValue(GPIO_HDMI_RX4_POWER)) {
		buf[0] = HDMI_RX_WAKEUP;
		INFO("hdmirx_5v A/B/C/D input lvl: %d : %d : %d : %d",
			xGpioGetValue(GPIO_HDMI_RX1_POWER),
			xGpioGetValue(GPIO_HDMI_RX2_POWER),
			xGpioGetValue(GPIO_HDMI_RX3_POWER),
			xGpioGetValue(GPIO_HDMI_RX4_POWER));
	}
	STR_Wakeup_src_Queue_Send_FromISR(buf);
}

void hdmirx_GpioIRQRegister(void)
{
	/* clear pinmux */
	xPinmuxSet(GPIO_HDMI_RX1_POWER, PIN_FUNC0);
	xPinmuxSet(GPIO_HDMI_RX2_POWER, PIN_FUNC0);
	xPinmuxSet(GPIO_HDMI_RX3_POWER, PIN_FUNC0);
	xPinmuxSet(GPIO_HDMI_RX4_POWER, PIN_FUNC0);

	xGpioSetDir(GPIO_HDMI_RX1_POWER, GPIO_DIR_IN);
	xGpioSetDir(GPIO_HDMI_RX2_POWER, GPIO_DIR_IN);
	xGpioSetDir(GPIO_HDMI_RX3_POWER, GPIO_DIR_IN);
	xGpioSetDir(GPIO_HDMI_RX4_POWER, GPIO_DIR_IN);

	INFO("hdmirx_5v A/B/C/D input lvl: %d : %d : %d : %d",
		xGpioGetValue(GPIO_HDMI_RX1_POWER),
		xGpioGetValue(GPIO_HDMI_RX2_POWER),
		xGpioGetValue(GPIO_HDMI_RX3_POWER),
		xGpioGetValue(GPIO_HDMI_RX4_POWER));

	xRequestGpioIRQ(GPIO_HDMI_RX1_POWER, hdmirx_IRQHandle, IRQF_TRIGGER_RISING);
	xRequestGpioIRQ(GPIO_HDMI_RX2_POWER, hdmirx_IRQHandle, IRQF_TRIGGER_RISING);
	xRequestGpioIRQ(GPIO_HDMI_RX3_POWER, hdmirx_IRQHandle, IRQF_TRIGGER_RISING);
	xRequestGpioIRQ(GPIO_HDMI_RX4_POWER, hdmirx_IRQHandle, IRQF_TRIGGER_RISING);
}

void hdmirx_GpioIRQFree(void)
{
	vFreeGpioIRQ(GPIO_HDMI_RX1_POWER);
	vFreeGpioIRQ(GPIO_HDMI_RX2_POWER);
	vFreeGpioIRQ(GPIO_HDMI_RX3_POWER);
	vFreeGpioIRQ(GPIO_HDMI_RX4_POWER);
	/* recovery pinmux */
	xPinmuxSet(GPIO_HDMI_RX1_POWER, PIN_FUNC1);
	xPinmuxSet(GPIO_HDMI_RX2_POWER, PIN_FUNC1);
	xPinmuxSet(GPIO_HDMI_RX3_POWER, PIN_FUNC1);
	xPinmuxSet(GPIO_HDMI_RX4_POWER, PIN_FUNC1);
}


