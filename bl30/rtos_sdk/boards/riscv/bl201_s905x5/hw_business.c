/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "hw_business.h"
#include "n200_eclic.h"
#include "n200_func.h"
#include "uart.h"
#include "eth.h"
#include "common.h"
#include "riscv_encoding.h"
#include "mailbox-api.h"
#include "hdmi_cec.h"
#include "stick_mem.h"
#include "suspend.h"
#include "vrtc.h"
#include "timer_source.h"

#define INT_TEST_NEST_DEPTH 6
#define INT_TEST_GPIO_NUM 6
#define INT_TEST_TASK_DELAY 50 // ms
#define TASK_TEST_STACK_DEPTH 200

//#define GPIO_INT_SOURCE(x) (SOC_PIC_INT_GPIO_BASE + x)

/* Configure board type:
 *   Board under test :        SIGNAL_BOARD_ENABLE     0
 *   Signal generation board : SIGNAL_BOARD_ENABLE     1
 */
#define SIGNAL_BOARD_ENABLE 0

#define INT_TEST_INT_WAVE_ENABLE 1

#if INT_TEST_INT_WAVE_ENABLE
#define INT_TEST_TIMER_PERIOD 500 // ms
#define INT_TEST_INT_DELAY 10 // ms
#else
#define INT_TEST_TIMER_PERIOD 500 // ms
#define INT_TEST_INT_DELAY 0x3ff // ms
#endif

#define INT_TEST_MAX_TIMER_PERIOD 100 // ms
#define INT_TEST_MIN_TIMER_PERIOD 50 // ms
#define INT_TEST_MUTE_TIMER_PERIOD 200 // ms

#define DMC_MON_CTRL0 ((0x0010  << 2) + 0xfe036000)
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
		if ((REG32(SYSCTRL_DEBUG_REG4) & MTE_FIX_STOP_BIT) == 1) //Stop MTE feature
			continue;
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

void config_eclic_irqs(void)
{
	eclic_init(ECLIC_NUM_INTERRUPTS);
	eclic_set_nlbits(0);
}

void hw_business_process(void)
{
	uint8_t i = 0;

	config_eclic_irqs();
#ifdef CONFIG_AOCPU_BUSRESPERR_DETECTION
	config_eclic_busresperr_irq();
#endif
	config_pmp();
	for (i = 0; i < 4; ++i)
		printf("AOCPU_IRQ_SEL=0x%x\n", REG32(AOCPU_IRQ_SEL0 + i * 4));
	stick_mem_init();
	stick_mem_write(STICK_REBOOT_FLAG, WATCHDOG_REBOOT);
	vMbInit();
	vCecCallbackInit(CEC_CHIP_S6);
	vRtcInit();
	//rtc_init();
	vETHMailboxCallback();
	create_str_task();
	create_mte_fix_task();
}
