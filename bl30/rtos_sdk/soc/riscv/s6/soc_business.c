/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "common.h"
#include "soc.h"
#include "n200_eclic.h"
#include "n200_func.h"
#include "riscv_encoding.h"
#include "timer_source.h"
#include "stick_mem.h"
#include "soc_business.h"

#define DMC_MON_CTRL0 ((0x0010 << 2) + 0xfe036000)
#define MTE_FIX_STOP_BIT 0x1
static TaskHandle_t MTEFixTask;

static uint8_t is_chip_version_a(void)
{
	if (((REG32(SYSCTRL_SEC_STATUS_REG0) >> 8) & 0xFF) == 0xA)
		return 1;
	else
		return 0;
}

static void vMTEFix(void *pvParameters)
{
	(void)pvParameters;
	uint32_t val;

	while (1) {
		if ((REG32(SYSCTRL_DEBUG_REG4) & MTE_FIX_STOP_BIT) == 1) { //Stop MTE feature
			vTaskDelay(pdMS_TO_TICKS(20));
			continue;
		}
		else {
			val = REG32(DMC_MON_CTRL0);
			val |= (1 << 30);
			REG32(DMC_MON_CTRL0) = val;
			udelay(10);
		}
	}
}

void create_mte_fix_task(void)
{
	if (!is_chip_version_a())
		return;

	if (xTaskCreate(vMTEFix, "MTEFix_Task", configMINIMAL_STACK_SIZE, NULL, 1, &MTEFixTask) < 0)
		printf("MTEFix_Task create fail!!\n");
}

void delete_mte_fix_task(void)
{
	if (!is_chip_version_a())
		return;

	if (MTEFixTask)
		vTaskDelete(MTEFixTask);
}

static void config_eclic_irqs(void)
{
	eclic_init(ECLIC_NUM_INTERRUPTS);
	eclic_set_nlbits(0);
}

void soc_business_process(void)
{
	config_eclic_irqs();
#ifdef CONFIG_AOCPU_BUSRESPERR_DETECTION
	config_eclic_busresperr_irq();
#endif
	config_pmp();
	stick_mem_init();
	stick_mem_write(STICK_REBOOT_FLAG, WATCHDOG_REBOOT);
	create_mte_fix_task();
}

#define PWR_STATE_WAIT_ON 16
void check_poweroff_status(void)
{
	const TickType_t xTimeout = pdMS_TO_TICKS(500); //Set timeout duration to 500ms
	TickType_t xStartTick;

	xStartTick = xTaskGetTickCount();

	/*Wait for cputop fsm switch to WAIT_ON*/
	while (((REG32(PWRCTRL_CPUTOP_FSM_STS0) >> 12) & 0x1F) != PWR_STATE_WAIT_ON) {
		if (xTaskGetTickCount() - xStartTick >= xTimeout) {
			printf("cputop fsm check timed out!\n");
			printf("PWRCTRL_CPUTOP_FSM_STS0: %x\n", REG32(PWRCTRL_CPUTOP_FSM_STS0));
			vTaskSuspend(NULL);
		}
	}
}
