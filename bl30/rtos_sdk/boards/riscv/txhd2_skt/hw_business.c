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
#include "mailbox-api.h"
#include "hdmi_cec.h"
#include "keypad.h"
#include "fsm.h"

#define INT_TEST_NEST_DEPTH  6
#define INT_TEST_GPIO_NUM  6
#define INT_TEST_TASK_DELAY  50 // ms
#define TASK_TEST_STACK_DEPTH  200

#define TASK_TEST_QUEUE_NUM  2
#define TASK_TEST_QUEUE_LENGTH  3

//#define GPIO_INT_SOURCE(x) (SOC_PIC_INT_GPIO_BASE + x)

/* Configure board type:
 *   Board under test :        SIGNAL_BOARD_ENABLE     0
 *   Signal generation board : SIGNAL_BOARD_ENABLE     1
 */
#define SIGNAL_BOARD_ENABLE       0

#define INT_TEST_INT_WAVE_ENABLE  1

#if INT_TEST_INT_WAVE_ENABLE
    #define INT_TEST_TIMER_PERIOD  500    // ms
    #define INT_TEST_INT_DELAY    10    // ms
#else
    #define INT_TEST_TIMER_PERIOD  500    // ms
    #define INT_TEST_INT_DELAY    0x3ff    // ms
#endif

#define INT_TEST_MAX_TIMER_PERIOD	100 // ms
#define INT_TEST_MIN_TIMER_PERIOD	50 // ms
#define INT_TEST_MUTE_TIMER_PERIOD	200 // ms

#define RTOS_BOOT_SUCC_REG		AO_DEBUG_REG2

/* Binary Semaphore */
QueueHandle_t xGPIOSemaphore[INT_TEST_NEST_DEPTH];
QueueHandle_t xMessageQueue[TASK_TEST_QUEUE_NUM];

void config_eclic_irqs(void)
{
	eclic_init(ECLIC_NUM_INTERRUPTS);
	eclic_set_nlbits(0);
}

void hw_business_process(void)
{
	uint8_t i = 0;

	// Initialize GPIOs, PIC and timer
	//vGPIOInit();
	config_eclic_irqs();
	for (i = 0; i < 8; ++i)
		printf("AOCPU_IRQ_SEL=0x%x\n", REG32(AOCPU_IRQ_SEL0 + i * 4));

	// Delay
	for (uint32_t i = 0; i < 0xffff; ++i)
		;

	vMbInit();
	vCoreFsmIdleInit();
	vCecCallbackInit(CEC_CHIP_TXHD2);
	vRtcInit();
	vETHMailboxCallback();
	create_str_task();
	vKeyPadCreate();
}

void aocpu_bringup_finished(void)
{
	#define RTOS_RUN_SUCC                  (1 << 0)
	*(volatile uint32_t *)RTOS_BOOT_SUCC_REG |= RTOS_RUN_SUCC;
}

