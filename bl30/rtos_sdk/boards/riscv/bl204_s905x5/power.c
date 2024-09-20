/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "common.h"
#include "gpio.h"
#include "ir.h"
#include "eth.h"
#include "soc.h"
#include "suspend.h"
#include "task.h"
#include "gpio.h"
#include "pwm.h"
#include "pwm_plat.h"
#include "keypad.h"
#include "timer_source.h"
#if CONFIG_WIFI_BT_WAKE
#include "wifi_bt_wake.h"
#endif
#include "power.h"
#include "mailbox-api.h"
#include "board_common.h"


#include "hdmi_cec.h"
static TaskHandle_t cecTask;

GE_GPIO_CTRL(VCC3V3_CSI_DVB, GPIOF_2, INVERT)
GE_GPIO_CTRL(VCC_5V, GPIOH_7, NOINVERT)
GE_GPIO_CTRL(VCC_5V_HDMI, GPIOH_6, NOINVERT)
HIZ_GPIO_CTRL(VCC_5V_USB, GPIOH_8)
GE_GPIO_CTRL(VDDCPU, GPIO_TEST_N, NOINVERT)

static int vdd_ee;
static int vdddos_npu_vpu;
static TaskHandle_t vadTask;

static struct IRPowerKey prvPowerKeyList[] = {
	{ 0xef10fe01, IR_NORMAL }, /* ref tv pwr */
	{ 0xba45bd02, IR_NORMAL }, /* small ir pwr */
	{ 0xef10fb04, IR_NORMAL }, /* old ref tv pwr */
	{ 0xf20dfe01, IR_NORMAL },
	{ 0xe51afb04, IR_NORMAL },
	{ 0xde217788, IR_NORMAL },
	{ 0x3ac5bd02, IR_CUSTOM },
	{}
};

static void vIRHandler(struct IRPowerKey *pkey)
{
	uint32_t buf[4] = { 0 };

	if (pkey->type == IR_NORMAL)
		buf[0] = REMOTE_WAKEUP;
	else if (pkey->type == IR_CUSTOM)
		buf[0] = REMOTE_CUS_WAKEUP;

	/* do sth below  to wakeup*/
	STR_Wakeup_src_Queue_Send_FromISR(buf);
};

static void *xMboxVadWakeup(void *msg)
{
	uint32_t buf[4] = { 0 };

	buf[0] = VAD_WAKEUP;
	STR_Wakeup_src_Queue_Send(buf);

	return NULL;
}

void str_hw_init(void)
{
	int ret;
	/*enable device & wakeup source interrupt*/
	vIRInit(MODE_HARD_NEC, GPIOF_3, PIN_FUNC1, prvPowerKeyList, ARRAY_SIZE(prvPowerKeyList),
		vIRHandler);
	vETHInit(0);

	xTaskCreate(vCEC_task, "CECtask", configMINIMAL_STACK_SIZE,
		    NULL, CEC_TASK_PRI, &cecTask);

	vBackupAndClearGpioIrqReg();
	vGpioIRQInit();
	vKeyPadInit();

#if CONFIG_WIFI_BT_WAKE
	wifi_bt_wakeup_init();
#endif
}

void str_hw_disable(void)
{
	/*disable wakeup source interrupt*/
	vIRDeint();
	vETHDeint();

	if (cecTask) {
		vTaskDelete(cecTask);
		cec_req_irq(0);
	}

#if CONFIG_WIFI_BT_WAKE
	wifi_bt_wakeup_deinit();
#endif

	vKeyPadDeinit();
	vRestoreGpioIrqReg();
}

void str_power_on(int shutdown_flag)
{
	int ret;

	(void)shutdown_flag;

	VDDCPU_on();

	/***set vdd_ee val***/
	ret = vPwmMesonsetvoltage(VDDEE_VOLT, vdd_ee);
	if (ret < 0) {
		printf("VDD_EE pwm set fail\n");
		return;
	}

	VCC3V3_CSI_DVB_on();
	VCC_5V_on();
	VCC_5V_USB_on();

	/*Wait POWERON_VDDCPU_DELAY for VDDCPU stable*/
	vTaskDelay(POWERON_VDDCPU_DELAY);

	printf("vdd_cpu on\n");
}

void str_power_off(int shutdown_flag)
{
	int ret;

	(void)shutdown_flag;

	VCC_5V_USB_off();
	VCC_5V_off();
	VCC3V3_CSI_DVB_off();

	if (shutdown_flag)
		VCC_5V_HDMI_off();
	/***set vdd_ee val***/
	vdd_ee = vPwmMesongetvoltage(VDDEE_VOLT);
	if (vdd_ee < 0) {
		printf("vdd_EE pwm get fail\n");
		return;
	}

	ret = vPwmMesonsetvoltage(VDDEE_VOLT, 710);
	if (ret < 0) {
		printf("vdd_EE pwm set fail\n");
		return;
	}

	/***power off A510 vdd_cpu***/
	VDDCPU_off();

	printf("Power down done.\n");
}
