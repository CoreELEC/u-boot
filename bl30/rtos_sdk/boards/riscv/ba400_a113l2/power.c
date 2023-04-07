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
#include "rtc.h"
#include "meson_i2c_slave.h"

/*#define SHOW_LATENCY */

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
#ifdef SHOW_LATENCY
uint32_t start_suspend_time;
uint32_t end_suspend_time;
uint32_t start_resume_time;
uint32_t end_resume_time;
#endif

static void vIRHandler(struct IRPowerKey *pkey)
{
	uint32_t buf[4] = { 0 };

	if (pkey->type == IR_NORMAL)
		buf[0] = REMOTE_WAKEUP;
	else if (pkey->type == IR_CUSTOM)
		buf[0] = REMOTE_CUS_WAKEUP;

#ifdef SHOW_LATENCY
	start_resume_time = timere_read_us();
#endif
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

#ifdef SHOW_LATENCY
	start_suspend_time = timere_read_us();
#endif
	/*enable device & wakeup source interrupt*/
	vIRInit(MODE_HARD_NEC, GPIOAO_1, PIN_FUNC3, prvPowerKeyList, ARRAY_SIZE(prvPowerKeyList),
		vIRHandler);
	vETHInit(0);

	vBackupAndClearGpioIrqReg();
	//vKeyPadInit();
	vGpioIRQInit();
	rtc_enable_irq();
}

void str_hw_disable(void)
{
	/*disable wakeup source interrupt*/
	vIRDeint();
	vETHDeint();

	//vKeyPadDeinit();
	vRestoreGpioIrqReg();
	rtc_disable_irq();
}

void str_power_on(int shutdown_flag)
{
	int ret;

	(void)shutdown_flag;

	/* open PWM clk */
	REG32(CLKCTRL_PWM_CLK_EF_CTRL) |= (1 << 24) | (1 << 8);

	/* set GPIOE_1 pinmux to pwm */
	xPinmuxSet(GPIOE_1, PIN_FUNC1);

	/* enable vddcpu PWM channel */
	REG32(PWMEF_MISC_REG_AB) |= (1 << 1);

	/***set vdd_cpu val***/
	ret = vPwmMesonsetvoltage(VDDCPU_VOLT, vdd_cpu);
	if (ret < 0) {
		printf("VDD_CPU pwm set fail\n");
		return;
	}

	/***power on vdd_cpu***/
	ret = xGpioSetDir(GPIO_TEST_N, GPIO_DIR_OUT);
	if (ret < 0) {
		printf("VDD_CPU set gpio dir fail\n");
		return;
	}

	ret = xGpioSetValue(GPIO_TEST_N, GPIO_LEVEL_HIGH);
	if (ret < 0) {
		printf("VDD_CPU set gpio val fail\n");
		return;
	}

	/***set vdd_ee val***/
	ret = vPwmMesonsetvoltage(VDDEE_VOLT, vdd_ee);
	if (ret < 0) {
		printf("VDD_EE pwm set fail\n");
		return;
	}

	if (shutdown_flag) {
		/***power on DDR_EN***/
		ret = xGpioSetDir(GPIOAO_0, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("DDR_EN set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIOAO_0, GPIO_LEVEL_HIGH);
		if (ret < 0) {
			printf("DDR_EN gpio val fail\n");
			return;
		}
	}

	/*HW need 1.5mS for VDDCPU statable, So SW add margin to 10ms*/
	vTaskDelay(pdMS_TO_TICKS(10));

	printf("vdd_cpu on\n");
#ifdef SHOW_LATENCY
	end_resume_time = timere_read_us();
	printf("BL30 system_suspend TS: %d  TE: %d\n", start_suspend_time, end_suspend_time);
	printf("BL30 system_resume TS: %d  TE: %d\n", start_resume_time, end_resume_time);
#endif
	/* this reset must excute immediately after power on because the wrapper is reseted*/
	if (shutdown_flag)
		watchdog_reset_system();
}

void str_power_off(int shutdown_flag)
{
	int ret;

	(void)shutdown_flag;

	if (shutdown_flag) {
		/***power off DDR_EN***/
		ret = xGpioSetDir(GPIOAO_0, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("DDR_EN set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIOAO_0, GPIO_LEVEL_LOW);
		if (ret < 0) {
			printf("DDR_EN gpio val fail\n");
			return;
		}
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
	ret = xGpioSetDir(GPIO_TEST_N, GPIO_DIR_OUT);
	if (ret < 0) {
		printf("vdd_cpu set gpio dir fail\n");
		return;
	}

	ret = xGpioSetValue(GPIO_TEST_N, GPIO_LEVEL_LOW);
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

	/*disable PWM CLK*/
	REG32(CLKCTRL_PWM_CLK_EF_CTRL) &= ~(1 << 24);

	/* disable PWM channel */
	REG32(PWMEF_MISC_REG_AB) &= ~(1 << 1);

	if (shutdown_flag) {
		vIRDeint();
		vIRInit(MODE_HARD_NEC_32K, GPIOAO_1, PIN_FUNC3, prvPowerKeyList,
			ARRAY_SIZE(prvPowerKeyList), vIRHandler);
		vIR32KInit(prvPowerKeyList[0].code, 0x00);
		/* set GPIOAO_0/1 pinmux to i2c slave */
		// xPinmuxSet(GPIOAO_0, PIN_FUNC1);
		// xPinmuxSet(GPIOAO_1, PIN_FUNC1);
		// xI2cSlaveMesonInit(0, 0);
	}

	printf("vdd_cpu off\n");

#ifdef SHOW_LATENCY
	end_suspend_time = timere_read_us();
#endif
}
