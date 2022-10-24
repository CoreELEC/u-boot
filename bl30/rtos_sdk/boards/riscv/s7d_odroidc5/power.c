/*
 * Copyright (c) 2025 Hardkernel Co, Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "common.h"
#include "eth.h"
#include "gpio.h"
#include "ir.h"
#include "power.h"
#include "rtc.h"
#include "soc.h"
#include "stick_mem.h"
#include "suspend.h"
#include "task.h"
#include "keypad.h"

#if BL30_SUSPEND_DEBUG_EN
#include "suspend_debug_s7d.h"
#endif

#include "hdmi_cec.h"

extern void vExtPhyInit(void);
extern void vExtPhyDeinit(void);

static TaskHandle_t cecTask;

#define VCC5V_GPIO		GPIOC_7
#define VDDCPU_A55_GPIO		GPIO_TEST_N

#define PWR_STATE_WAIT_ON	16

static int vdddos_npu_vpu;
static TaskHandle_t vadTask;

static struct IRPowerKey prvPowerKeyList[] = {
	{ 0x23dc4db2, IR_NORMAL },	/* Hardkernel Stock Remote Controller */
	{ 0x00000000, IR_CUSTOM },
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
	while (((REG32(PWRCTRL_CPUTOP_FSM_STS0) >> 12) & 0x1F) != PWR_STATE_WAIT_ON) {
		if (xTaskGetTickCount() - xStartTick >= xTimeout) {
			printf("cputop fsm check timed out!\n");
			printf("PWRCTRL_CPUTOP_FSM_STS0: %x\n", REG32(PWRCTRL_CPUTOP_FSM_STS0));
			printf("PWRCTRL_CPU0_FSM_STS0: %x\n", REG32(PWRCTRL_CPU0_FSM_STS0));
			printf("PWRCTRL_CPU1_FSM_STS0: %x\n", REG32(PWRCTRL_CPU1_FSM_STS0));
			printf("PWRCTRL_CPU2_FSM_STS0: %x\n", REG32(PWRCTRL_CPU2_FSM_STS0));
			printf("PWRCTRL_CPU3_FSM_STS0: %x\n", REG32(PWRCTRL_CPU3_FSM_STS0));
			vTaskSuspend(NULL);
		}
	}
}

void str_hw_init(void)
{
	int ret;

	vIRInit(MODE_HARD_NEC, GPIODV_0, PIN_FUNC1, prvPowerKeyList,
			ARRAY_SIZE(prvPowerKeyList), vIRHandler);

	xTaskCreate(vCEC_task, "CECtask", configMINIMAL_STACK_SIZE,
		    NULL, CEC_TASK_PRI, &cecTask);

	vBackupAndClearGpioIrqReg();
	vGpioIRQInit();
	vExtPhyInit();
	vGpioKeyEnable();

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
	vIRDeint();

	if (cecTask) {
		vTaskDelete(cecTask);
		cec_req_irq(0);
	}

	vGpioKeyDisable();
	vExtPhyDeinit();
	vRestoreGpioIrqReg();

#if BL30_SUSPEND_DEBUG_EN
	exit_func_print();
#endif
}

void str_power_on(int shutdown_flag)
{
	int ret;

	(void)shutdown_flag;
#if BL30_SUSPEND_DEBUG_EN
	enter_func_print();
	if (!IS_EN(BL30_SKIP_POWER_SWITCH)) {
#endif
		/***power on A55 vdd_cpu***/
		ret = xGpioSetDir(VDDCPU_A55_GPIO, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("vdd_cpu set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(VDDCPU_A55_GPIO, GPIO_LEVEL_HIGH);
		if (ret < 0) {
			printf("vdd_cpu set gpio val fail\n");
			return;
		}

		if (shutdown_flag) {
			/***power on vcc_3.3v***/

			//ret = xGpioSetDir(VCC3V3_GPIO, GPIO_DIR_OUT);
			//if (ret < 0) {
			//	printf("vcc_3.3v set gpio dir fail\n");
			//	return;
			//}

			//ret = xGpioSetValue(VCC3V3_GPIO, GPIO_LEVEL_HIGH);
			//if (ret < 0) {
			//	printf("vcc_3.3v gpio val fail\n");
			//	return;
			//}

		}

		/***power on vcc_5v***/
		ret = xGpioSetDir(VCC5V_GPIO, GPIO_DIR_IN);
		if (ret < 0) {
			printf("vcc_5v set gpio dir fail\n");
			return;
		}

		/*Wait POWERON_VDDCPU_DELAY for VDDCPU stable*/
		vTaskDelay(POWERON_VDDCPU_DELAY);

		printf("vdd_cpu on\n");
#if BL30_SUSPEND_DEBUG_EN
	}
	/* size over load */
	dump_cpu_fsm_regs();
	show_pwm_regs();
	exit_func_print();
#endif
}

void str_power_off(int shutdown_flag)
{
	int ret;

	(void)shutdown_flag;
#if BL30_SUSPEND_DEBUG_EN
	enter_func_print();
	if (!IS_EN(BL30_SKIP_POWER_SWITCH)) {
#endif
		/***power off vcc_5v***/
		if (get_USB_Power_flag() == 0) {
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
		}

		/***power off A55 vdd_cpu***/
		ret = xGpioSetDir(VDDCPU_A55_GPIO, GPIO_DIR_OUT);
		if (ret < 0) {
			printf("vdd_cpu set gpio dir fail\n");
			return;
		}

		ret = xGpioSetValue(VDDCPU_A55_GPIO, GPIO_LEVEL_LOW);
		if (ret < 0) {
			printf("vdd_cpu set gpio val fail\n");
			return;
		}

		printf("Power down done.\n");
#if BL30_SUSPEND_DEBUG_EN
	} else
		printf("skiped power switch...\n");
	dump_cpu_fsm_regs();
	show_pwm_regs();
	exit_func_print();
#endif
}
