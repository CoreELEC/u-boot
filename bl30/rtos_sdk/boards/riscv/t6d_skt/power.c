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
#include "wakeup.h"
#include "power.h"
#include "mailbox-api.h"
#include "hdmi_cec.h"
#include "hdmirx_wake.h"
#include "pm.h"
#include "suspend_debug.h"
#if BL30_SUSPEND_DEBUG_EN
#include "suspend_debug_t6d.h"
#endif
#define CONFIG_HDMIRX_PLUGIN_WAKEUP

static TaskHandle_t cecTask;
#if BL30_SUSPEND_DEBUG_EN
static TaskHandle_t printTask;
#endif

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

#if BL30_SUSPEND_DEBUG_EN
void vPrintTask(void *pvParameters)
{
	/*make compiler happy*/
	(void) pvParameters;

	for ( ;; ) {
		printf("vPTask1...\n");
		vTaskDelay(pdMS_TO_TICKS(TEST_TASK1_DELAY));
	}
}
#endif

void str_hw_init(void)
{
	int ret;

#if BL30_SUSPEND_DEBUG_EN
	enter_func_print();
	/*enable device & wakeup source interrupt*/
	if (!IS_EN(BL30_IR_WAKEUP_MASK))
#endif
		vIRInit(MODE_HARD_NEC, GPIOD_5, PIN_FUNC1, prvPowerKeyList,
			ARRAY_SIZE(prvPowerKeyList), vIRHandler);
#if BL30_SUSPEND_DEBUG_EN
	else
		printf("skiped IR wakeup function\n");

	if (IS_EN(BL30_RTC_WAKEUP_MASK)) {
		printf("skiped RTC wakeup function\n");
		alarm_clr();
	}
#endif

	vETHInit(0);

	xTaskCreate(vCEC_task, "CECtask", configMINIMAL_STACK_SIZE,
		    NULL, CEC_TASK_PRI, &cecTask);

	vBackupAndClearGpioIrqReg();
#if BL30_SUSPEND_DEBUG_EN
	if (!IS_EN(BL30_SARADC_WAKEUP_MASK))
#endif
		vKeyPadInit();
#if BL30_SUSPEND_DEBUG_EN
	else
		printf("skiped SARADC wakeup function\n");
#endif
	vGpioIRQInit();
	Bt_GpioIRQRegister();
#ifdef CONFIG_HDMIRX_PLUGIN_WAKEUP
	/*ctrl of 5v wake up*/
	if (REG32(TOP_EDID_RAM_OVR0_DATA) & 0x1)
		hdmirx_GpioIRQRegister();
#endif
#if BL30_SUSPEND_DEBUG_EN
	exit_func_print();
#endif
}

void str_hw_disable(void)
{
#if BL30_SUSPEND_DEBUG_EN
	enter_func_print();
#endif
	/*disable wakeup source interrupt*/
#if BL30_SUSPEND_DEBUG_EN
	if (!IS_EN(BL30_IR_WAKEUP_MASK))
#endif
		vIRDeint();

	vETHDeint();

#if BL30_SUSPEND_DEBUG_EN
	if (!IS_EN(BL30_SARADC_WAKEUP_MASK))
#endif
		vKeyPadDeinit();
	vRestoreGpioIrqReg();

	if (cecTask) {
		vTaskDelete(cecTask);
		cec_req_irq(0);
	}
	Bt_GpioIRQFree();
#ifdef CONFIG_HDMIRX_PLUGIN_WAKEUP
	/*ctrl of 5v wake up*/
	if (REG32(TOP_EDID_RAM_OVR0_DATA) & 0x1)
		hdmirx_GpioIRQFree();
#endif
#if BL30_SUSPEND_DEBUG_EN
	exit_func_print();
#endif
}

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
#if BL30_SUSPEND_DEBUG_EN
	enter_func_print();
	/* open PWM clk */
//	REG32(CLKCTRL_PWM_CLK_EF_CTRL) |= (1 << 24) | (1 << 8);
	if (!IS_EN(BL30_SKIP_POWER_SWITCH)) {
#endif
		/***restore vdd_ee val***/
		ret = vPwmMesonsetvoltage(VDDEE_VOLT, vdd_ee);
		if (ret < 0) {
			printf("VDD_EE pwm set fail\n");
			return;
		}
		/* set GPIOE_1 pinmux to pwm */
		xPinmuxSet(GPIOE_1, PIN_FUNC1);

		/* enable vddcpu PWM channel */
		REG32(PWM_MISC_REG_A) |= (1 << 1);

		/***set vdd_cpu val***/
		ret = vPwmMesonsetvoltage(VDDCPU_VOLT, vdd_cpu);
		if (ret < 0) {
			printf("VDD_CPU pwm set fail\n");
			return;
		}

		/***power on vdd_cpu***/
		ret = xGpioSetDir(GPIOD_14, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("vdd_cpu set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIOD_14, GPIO_LEVEL_HIGH);
		if (ret < 0) {
			printf("vdd_cpu set gpio val fail\n");
			return;
		}

		if (shutdown_flag) {
			/***power on WOL&VAD*** vddq power issue*/
/*			ret = xGpioSetDir(GPIOD_4, GPIO_DIR_OUT);
 *			if (ret < 0) {
 *				printf("vad_en set gpio dir fail\n");
 *				return;
 *			}
 *
 *			ret = xGpioSetValue(GPIOD_4, GPIO_LEVEL_HIGH);
 *			if (ret < 0) {
 *				printf("vad_en gpio val fail\n");
 *				return;
 *			}
 */
		}
		/***power on vcc_3v3***/
		ret = xGpioSetDir(GPIOD_10, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("vcc_3.3v set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIOD_10, GPIO_LEVEL_HIGH);
		if (ret < 0) {
			printf("vcc_3.3v gpio val fail\n");
			return;
		}

		/***power on vcc_5v***/
		ret = xGpioSetDir(GPIO_TEST_N, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("vcc_5v set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIO_TEST_N, GPIO_LEVEL_HIGH);
		if (ret < 0) {
			printf("vcc_5v gpio val fail\n");
			return;
		}

		/*Wait POWERON_VDDCPU_DELAY for VDDCPU stable*/
		vTaskDelay(POWERON_VDDCPU_DELAY);

		printf("vdd_cpu on\n");
		str_gpio_restore();
#if BL30_SUSPEND_DEBUG_EN
	}

	/* size over load */
	dump_cpu_fsm_regs();
	stop_debug_task();
	exit_func_print();
#endif
}

void str_power_off(int shutdown_flag)
{
	int ret;

	str_gpio_backup();

	(void)shutdown_flag;
#if BL30_SUSPEND_DEBUG_EN
	enter_func_print();
	start_debug_task();
	if (!IS_EN(BL30_SKIP_POWER_SWITCH)) {
#endif
		/***power off vcc_5v***/
		ret = xGpioSetDir(GPIO_TEST_N, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("vcc_5v set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIO_TEST_N, GPIO_LEVEL_LOW);
		if (ret < 0) {
			printf("vcc_5v gpio val fail\n");
			return;
		}

		if (shutdown_flag) {
			/***power off wol &vad*** cause vddq issue*/
/*			ret = xGpioSetDir(GPIOD_4, GPIO_DIR_OUT);
 *			if (ret < 0) {
 *				printf("vad_en set gpio dir fail\n");
 *				return;
 *			}
 *
 *			ret = xGpioSetValue(GPIOD_4, GPIO_LEVEL_LOW);
 *			if (ret < 0) {
 *				printf("vad_en gpio val fail\n");
 *				return;
 *			}
 */
		}
		/***power off vcc_3v3***/
		ret = xGpioSetDir(GPIOD_10, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("vcc_3.3v set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIOD_10, GPIO_LEVEL_LOW);
		if (ret < 0) {
			printf("vcc_3.3v gpio val fail\n");
			return;
		}

		/***set vdd_cpu val***/
		vdd_cpu = vPwmMesongetvoltage(VDDCPU_VOLT);
		if (vdd_ee < 0) {
			printf("VDD_CPU pwm get fail\n");
			return;
		}

		/***power off vdd_cpu***/
		ret = xGpioSetDir(GPIOD_14, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("vdd_cpu set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(GPIOD_14, GPIO_LEVEL_LOW);
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
//		REG32(CLKCTRL_PWM_CLK_EF_CTRL) &= ~(1 << 24)

		/* disable PWM channel */
		REG32(PWM_MISC_REG_A) &= ~(1 << 1);

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

		printf("vdd_cpu off\n");
#if BL30_SUSPEND_DEBUG_EN
	} else
		printf("skiped power switch\n");
	show_pwm_regs();
	exit_func_print();
#endif
	str_gpio_restore();
}

static int platform_power_begin(void)
{
	logi("%s, power off\n", __func__);
	return 0;
}

static int platform_power_end(void)
{
	logi("%s, power on\n", __func__);
	return 0;
}

static struct platform_power_ops ops = {
	.begin = platform_power_begin,
	.end = platform_power_end,
};

void platform_power_interface_register(void)
{
	set_platform_power_ops(&ops);
}
