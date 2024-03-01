/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "sw_business.h"
#include "register.h"
#ifdef CONFIG_PM
#include "pm.h"
#endif
#if defined(CONFIG_RISCV) && !defined(CONFIG_SOC_OLD_ARCH) && !defined(CONFIG_BRINGUP)
#include "uart.h"
#endif

#define BRINGUP_TEST	(0)

#if BRINGUP_TEST
#include <stdio.h>
#include "FreeRTOS.h"
#include "FreeRTOS.h"
#include "timers.h"

#define TEST_TIMER_PERIOD		(500)	// ms
#define TEST_TASK1_DELAY		(1000)	// ms
#define TEST_TASK2_DELAY		(500)	// ms

/* Timer handle */
TimerHandle_t xSoftTimer = NULL;

static void vPrintSystemStatus(TimerHandle_t xTimer) {
	xTimer = xTimer;
	taskENTER_CRITICAL();
	printf("Timer ...\n");
	taskEXIT_CRITICAL();
}

static void vPrintTask1( void *pvParameters )
{
	/*make compiler happy*/
	pvParameters = pvParameters;

	for ( ;; )
	{
		printf("vPTask1...\n");
		vTaskDelay(pdMS_TO_TICKS(TEST_TASK1_DELAY));
	}
}

static void vPrintTask2( void *pvParameters )
{
	/*make compiler happy*/
	pvParameters = pvParameters;

	vTaskDelay(pdMS_TO_TICKS(TEST_TASK2_DELAY));
	for ( ;; )
	{
		printf("vPTask2...\n");
		vTaskDelay(pdMS_TO_TICKS(TEST_TASK2_DELAY));
	}
}
#endif

void sw_business_process(void)
{
#if BRINGUP_TEST
	// Create timer
	xSoftTimer = xTimerCreate("Timer", pdMS_TO_TICKS(TEST_TIMER_PERIOD), pdTRUE, NULL, vPrintSystemStatus);

	printf("Starting timer ...\n");
	xTimerStart(xSoftTimer, 0);

	xTaskCreate( vPrintTask1, "Print1", configMINIMAL_STACK_SIZE, NULL, 2, NULL );
	xTaskCreate( vPrintTask2, "Print2", configMINIMAL_STACK_SIZE, NULL, 2, NULL );
#endif
#ifdef CONFIG_PM
	find_static_power_dev();
#endif
#if defined(CONFIG_RISCV) && !defined(CONFIG_SOC_OLD_ARCH) && !defined(CONFIG_BRINGUP)
	if (*(volatile uint32_t *)(SYSCTRL_SEC_STATUS_REG4) & ACS_DIS_PRINT_FLAG)
		vBL30PrintControlInit();
#endif
}

