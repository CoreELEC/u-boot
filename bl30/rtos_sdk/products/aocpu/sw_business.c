/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "sw_business.h"

#define BRINGUP_TEST    (0)

#if BRINGUP_TEST
#include <stdio.h>
#include "FreeRTOS.h"
#include "FreeRTOS.h"
#include "timers.h"

#define INT_TEST_INT_WAVE_ENABLE  1

#if INT_TEST_INT_WAVE_ENABLE
    #define INT_TEST_TIMER_PERIOD  500    // ms
    #define INT_TEST_INT_DELAY    10      // ms
#else
    #define INT_TEST_TIMER_PERIOD  500     // ms
    #define INT_TEST_INT_DELAY    0x3ff    // ms
#endif

#define INT_TEST_MAX_TIMER_PERIOD	100    // ms
#define INT_TEST_MIN_TIMER_PERIOD	50     // ms
#define INT_TEST_MUTE_TIMER_PERIOD	200    // ms

/* Timer handle */
TimerHandle_t xSoftTimer = NULL;

static void vPrintSystemStatus(TimerHandle_t xTimer) {
	xTimer = xTimer;
	taskENTER_CRITICAL();
	printf("\nTimer ...\n");
	taskEXIT_CRITICAL();
}

static void vPrintTask1( void *pvParameters )
{
    /*make compiler happy*/
    pvParameters = pvParameters;

	for ( ;; )
	{
		printf("\nvPTask1 %d\n");
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

static void vPrintTask2( void *pvParameters )
{
    /*make compiler happy*/
    pvParameters = pvParameters;
	vTaskDelay(pdMS_TO_TICKS(50));
	for ( ;; )
	{
		printf("\nvPTask2 %d\n");
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}
#endif

void sw_business_process(void)
{
#if BRINGUP_TEST
	// Create timer
	xSoftTimer = xTimerCreate("Timer", pdMS_TO_TICKS(INT_TEST_TIMER_PERIOD), pdTRUE, NULL, vPrintSystemStatus);

	printf("Starting timer ...\n");
	xTimerStart(xSoftTimer, 0);

	xTaskCreate( vPrintTask1, "Print1", configMINIMAL_STACK_SIZE, NULL, 2, NULL );
	xTaskCreate( vPrintTask2, "Print2", configMINIMAL_STACK_SIZE, NULL, 2, NULL );
#endif
}

