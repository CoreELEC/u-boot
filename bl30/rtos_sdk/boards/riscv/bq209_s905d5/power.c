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
#include "wakeup.h"
#include "power.h"
#include "mailbox-api.h"
#include "board_common.h"
#include "stick_mem.h"

#include "hdmi_cec.h"
static TaskHandle_t cecTask;

GE_GPIO_CTRL(VCC_5V, GPIOH_7, NOINVERT)
HIZ_GPIO_CTRL(VCC_5V_USB, GPIOH_8)
GE_GPIO_CTRL(VCC_5V_HDMI, GPIOH_6, NOINVERT)
GE_GPIO_CTRL(VDDCPU, GPIO_TEST_N, NOINVERT)
GE_GPIO_CTRL(VCC3V3, GPIOD_6, NOINVERT)
GE_GPIO_CTRL(VCC3V3_CARD, GPIOD_5, NOINVERT)
GE_GPIO_CTRL(VCC3V3_LCD, GPIOA_4, NOINVERT)
GE_GPIO_CTRL(VCC3V3_CM, GPIOA_13, NOINVERT)
GE_GPIO_CTRL(ETH_RESET, GPIOZ_15, NOINVERT)


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

	stick_mem_write(STICK_IR_WAKEUP_KEY, pkey->code);
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
	//Bt_GpioIRQRegister();
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
	//Bt_GpioIRQFree();
	vKeyPadDeinit();
	vRestoreGpioIrqReg();
}

#define STEP_VOL	30  // 30mV steps

static int __vddee_and_vddddr_ctrl(int *cur_vol, int target)
{
	int ret;
	int ee_vol, ddr_vol;
	int step, remain, loop;

	/* 1. If VDD_DDR does not exist, only set the vol of vddee */
	ee_vol = vPwmMesongetvoltage(VDDEE_VOLT);
	if (ee_vol < 0) {
		printf("vdd_EE pwm get fail\n");
		*cur_vol = 0;
		return -1;
	}
	*cur_vol = ee_vol;

	ddr_vol = vPwmMesongetvoltage(VDDDDR_VOLT);
	if (ddr_vol < 0) {
		printf("VDD DDR pwm get fail\n");
		printf("Only set the vddee.\n");
		ret = vPwmMesonsetvoltage(VDDEE_VOLT, target);
		if (ret < 0) {
			printf("vdd_EE pwm set fail\n");
			return -1;
		}
		goto DONE;
	}

	/* 2. Adjust target voltage, step by step.
	 * NOTE: Assumes VDDEE and VDDDDR voltages are the same.
	 */
	step = (ee_vol > target) ? 0 - STEP_VOL : STEP_VOL;

	while (ee_vol != target) {
		ee_vol += step;
		if (step < 0)
			ee_vol = ee_vol < target ? target : ee_vol;
		else
			ee_vol = ee_vol > target ? target : ee_vol;

		ret = vPwmMesonsetvoltage(VDDEE_VOLT, ee_vol);
		if (ret < 0) {
			printf("vdd_EE pwm set fail\n");
			return -1;
		}

		ret = vPwmMesonsetvoltage(VDDDDR_VOLT, ee_vol);
		if (ret < 0) {
			printf("vdd_DDR pwm set fail\n");
			return -1;
		}
//		printf("ee&ddr cur vol [%d], target vol [%d]\n", ee_vol, target);
//		vTaskDelay(pdMS_TO_TICKS(3000));
	}

DONE:
	return 0;
}

static int vdd_ee;
#define VDDEE_STR_VOLT	710 // 710mv for tsmc 6nm

static int vddee_and_vddddr_ctrl(int is_suspend)
{
	if (is_suspend)
		return __vddee_and_vddddr_ctrl(&vdd_ee, VDDEE_STR_VOLT);
	else
		return __vddee_and_vddddr_ctrl(&vdd_ee, vdd_ee);
}

void str_power_on(int shutdown_flag)
{
	int ret;

	(void)shutdown_flag;
	VDDCPU_on();

	vddee_and_vddddr_ctrl(0);


	VCC3V3_CM_on();
	VCC3V3_LCD_on();
	VCC3V3_on();
	VCC3V3_CARD_on();
	VCC_5V_on();
	VCC_5V_USB_on();

	if (exeth_wol_n_flag) {
		printf("exeth power on\n");
		ETH_RESET_on();
	}

	/*Wait POWERON_VDDCPU_DELAY for VDDCPU stable*/
	vTaskDelay(POWERON_VDDCPU_DELAY);

	printf("vdd_cpu on\n");
}

void str_power_off(int shutdown_flag)
{
	int ret;

	(void)shutdown_flag;

	if (exeth_wol_n_flag) {
		printf("exeth wol set\n");
		ETH_RESET_off();
	}

	VCC_5V_USB_off();
	VCC_5V_off();
	VCC3V3_CARD_off();
	VCC3V3_off();
	VCC3V3_LCD_off();
	VCC3V3_CM_off();

	if (shutdown_flag)
		VCC_5V_HDMI_off();
	vddee_and_vddddr_ctrl(1);

	/***power off A510 vdd_cpu***/
	VDDCPU_off();
	printf("Power down done.\n");
}
