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
#include "suspend.h"
#include "task.h"
#include "gpio.h"
#include "pwm.h"
#include "pwm_plat.h"
#include "keypad.h"
#include "timer_source.h"
#include "vad_suspend.h"
#include "wakeup.h"
#include "power.h"
#include "mailbox-api.h"
#include "hdmi_cec.h"
static TaskHandle_t cecTask;

static int vdd_ee;
static int vdd_cpu;
static TaskHandle_t vadTask;

static struct IRPowerKey prvPowerKeyList[] = {
	{ 0xef10fe01, IR_NORMAL }, /* ref tv pwr */
	{ 0xba45bd02, IR_NORMAL }, /* small ir pwr */
	{ 0xef10fb04, IR_NORMAL }, /* old ref tv pwr */
	{ 0xf20dfe01, IR_NORMAL },
	{ 0xe51afb04, IR_NORMAL },
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

void check_poweroff_status(void)
{
	const TickType_t xTimeout = pdMS_TO_TICKS(500);	//Set timeout duration to 500ms
	TickType_t xStartTick;

	xStartTick = xTaskGetTickCount();

	/*Wait for cputop fsm switch to WAIT_ON*/
	while (((REG32(PWRCTRL_CPUTOP_FSM_STS0) >> 12) & 0x1F) != 16) {
		if (xTaskGetTickCount() - xStartTick >= xTimeout) {
			printf("cputop fsm check timed out!\n");
			printf("PWRCTRL_CPUTOP_FSM_STS0: %x\n", REG32(PWRCTRL_CPUTOP_FSM_STS0));
			printf("PWRCTRL_CPU0_FSM_STS0: %x\n", REG32(PWRCTRL_CPU0_FSM_STS0));
			printf("PWRCTRL_CPU1_FSM_STS0: %x\n", REG32(PWRCTRL_CPU1_FSM_STS0));
			printf("PWRCTRL_CPU2_FSM_STS0: %x\n", REG32(PWRCTRL_CPU2_FSM_STS0));
			printf("PWRCTRL_CPU3_FSM_STS0: %x\n", REG32(PWRCTRL_CPU3_FSM_STS0));
			printf("PWRCTRL_CPU4_FSM_STS0: %x\n", REG32(PWRCTRL_CPU4_FSM_STS0));
			vTaskSuspend(NULL);
		}
	}
}

void str_hw_init(void)
{
	int ret;
	/*enable device & wakeup source interrupt*/
	vIRInit(MODE_HARD_NEC, GPIOD_5, PIN_FUNC1, prvPowerKeyList, ARRAY_SIZE(prvPowerKeyList),
		vIRHandler);
	vETHInit(0);

	xTaskCreate(vCEC_task, "CECtask", configMINIMAL_STACK_SIZE,
		    NULL, CEC_TASK_PRI, &cecTask);


	vBackupAndClearGpioIrqReg();
	vGpioIRQInit();
	vKeyPadInit();
	Bt_GpioIRQRegister();
}

void str_hw_disable(void)
{
	/*disable wakeup source interrupt*/
	vIRDeint();
	vETHDeint();

	vKeyPadDeinit();
	vRestoreGpioIrqReg();

	if (cecTask) {
		vTaskDelete(cecTask);
		cec_req_irq(0);
	}
	Bt_GpioIRQFree();
}

#define VCC5V_GPIO	GPIO_TEST_N
/* A55 and A76 share with dc/dc en in SKT board */
/* The DCDC of VCC3V3 and VDDCPU share the same enable pin.*/
#define VDDCPU_GPIO	GPIOD_10 // Share with VCC3V3
#define WOL_EN_GPIO	GPIOD_14

static void str_gpio_backup(void)
{
	//TODO:

	//Example:
	//if (xBankStateBackup("A"))
	//	printf("xBankStateBackup fail\n");
}

static void str_gpio_restore(void)
{
	//TODO:

	//Example:
	//if (xBankStateRestore("A"))
	//	printf("xBankStateRestore fail\n");
}

void str_power_on(int shutdown_flag)
{
	int ret;

	(void)shutdown_flag;
	/* open PWM clk */
//	REG32(CLKCTRL_PWM_CLK_EF_CTRL) |= (1 << 24) | (1 << 8);

	/* set GPIOE_1 pinmux to pwm */
	xPinmuxSet(GPIOE_1, PIN_FUNC1);
	xPinmuxSet(GPIOE_2, PIN_FUNC1);

	/* enable vddcpu PWM channel */
	REG32(PWMAB_MISC_REG_AB) |= (1 << 1);
	REG32(PWMEF_MISC_REG_AB) |= (1 << 0);

	/***set vdd_cpu val***/
	ret = vPwmMesonsetvoltage(VDDCPU_VOLT, vdd_cpu);
	if (ret < 0) {
		printf("VDD_CPU pwm set fail\n");
		return;
	}

	/***power on vdd_cpu***/
	ret = xGpioSetDir(VDDCPU_GPIO, GPIO_DIR_OUT);
	if (ret < 0) {
		printf("vdd_cpu set gpio dir fail\n");
		return;
	}

	ret = xGpioSetValue(VDDCPU_GPIO, GPIO_LEVEL_HIGH);
	if (ret < 0) {
		printf("vdd_cpu set gpio val fail\n");
		return;
	}

	/***set vdd_ee val***/
	ret = vPwmMesonsetvoltage(VDDEE_VOLT, vdd_ee);
	if (ret < 0) {
		printf("VDD_EE pwm set fail\n");
		return;
	}
	/***power off wol en***/
	ret = xGpioSetDir(WOL_EN_GPIO, GPIO_DIR_OUT);
	if (ret < 0) {
		printf("wol en pin set gpio dir fail\n");
		return;
	}
	if (get_ETHWol_flag() == 0) {
		ret = xGpioSetValue(WOL_EN_GPIO, GPIO_LEVEL_HIGH);
		if (ret < 0) {
			printf("wol en pin gpio val fail\n");
			return;
		}
	}

	/***power on vcc_5v***/
	ret = xGpioSetDir(VCC5V_GPIO, GPIO_DIR_OUT);
	if (ret < 0) {
		printf("vcc_5v set gpio dir fail\n");
		return;
	}

	ret = xGpioSetValue(VCC5V_GPIO, GPIO_LEVEL_HIGH);
	if (ret < 0) {
		printf("vcc_5v gpio val fail\n");
		return;
	}

	if (shutdown_flag) {
		/***power on VDDQ***/
		ret = xGpioSetDir(GPIOD_4, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("VDDQ set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIOD_4, GPIO_LEVEL_HIGH);
		if (ret < 0) {
			printf("VDDQ set gpio val fail\n");
			return;
		}
	}

	/*Wait POWERON_VDDCPU_DELAY for VDDCPU stable*/
	vTaskDelay(POWERON_VDDCPU_DELAY);

	printf("vdd_cpu on\n");

	str_gpio_restore();
}

void str_power_off(int shutdown_flag)
{
	int ret;

	str_gpio_backup();

	(void)shutdown_flag;

	if (shutdown_flag) {
		/***power off VDDQ***/
		ret = xGpioSetDir(GPIOD_4, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("VDDQ set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIOD_4, GPIO_LEVEL_LOW);
		if (ret < 0) {
			printf("VDDQ set gpio val fail\n");
			return;
		}
	}

	/***power off vcc_5v***/
	ret = xGpioSetDir(VCC5V_GPIO, GPIO_DIR_OUT);
	if (ret < 0) {
		printf("vcc_5v set gpio dir fail\n");
		return;
	}

	ret = xGpioSetValue(VCC5V_GPIO, GPIO_LEVEL_LOW);
	if (ret < 0) {
		printf("vcc_5v gpio val fail\n");
		return;
	}

	/***set vdd_ee val***/
	vdd_ee = vPwmMesongetvoltage(VDDEE_VOLT);
	if (vdd_ee < 0) {
		printf("vdd_EE pwm get fail\n");
		return;
	}

	ret = vPwmMesonsetvoltage(VDDEE_VOLT, 770);
	if (ret < 0) {
		printf("vdd_EE pwm set fail\n");
		return;
	}

	/***set vdd_cpu val***/
	vdd_cpu = vPwmMesongetvoltage(VDDCPU_VOLT);
	if (vdd_ee < 0) {
		printf("VDD_CPU pwm get fail\n");
		return;
	}

	/***power off vdd_cpu***/
	ret = xGpioSetDir(VDDCPU_GPIO, GPIO_DIR_OUT);
	if (ret < 0) {
		printf("vdd_cpu set gpio dir fail\n");
		return;
	}

	ret = xGpioSetValue(VDDCPU_GPIO, GPIO_LEVEL_LOW);
	if (ret < 0) {
		printf("vdd_cpu set gpio val fail\n");
		return;
	}

	/* set GPIOE_1 pinmux to gpio */
	xPinmuxSet(GPIOE_1, PIN_FUNC0);

	/***set vddcpu pwm to input***/
	ret = xGpioSetDir(GPIOE_1, GPIO_DIR_IN);
	if (ret < 0) {
		printf("GPIOE_1 set gpio dir fail\n");
		return;
	}

	/* set GPIOE_2 pinmux to gpio */
	xPinmuxSet(GPIOE_2, PIN_FUNC0);

	/***set vddcpu pwm to input***/
	ret = xGpioSetDir(GPIOE_2, GPIO_DIR_IN);
	if (ret < 0) {
		printf("GPIOE_2 set gpio dir fail\n");
		return;
	}


	/*disable PWM CLK*/
//	REG32(CLKCTRL_PWM_CLK_EF_CTRL) &= ~(1 << 24);

	/* disable PWM channel */
	REG32(PWMAB_MISC_REG_AB) &= ~(1 << 1);
	REG32(PWMEF_MISC_REG_AB) &= ~(1 << 0);

	printf("vdd_cpu off\n");
}
