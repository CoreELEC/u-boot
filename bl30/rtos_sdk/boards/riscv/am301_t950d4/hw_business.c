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
#include "n200_pic_tmr.h"
#include "n200_func.h"
#include "uart.h"
#include "eth.h"
#include "common.h"
#include "mailbox-api.h"
#include "hdmi_cec.h"
#include "stick_mem.h"
#include "keypad.h"
#include "fsm.h"
#include "sdk_ver.h"

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

/* Binary Semaphore */
QueueHandle_t xGPIOSemaphore[INT_TEST_NEST_DEPTH];
QueueHandle_t xMessageQueue[TASK_TEST_QUEUE_NUM];

/* function: vPICInit */
static void vPICInit(void)
{
	// Disable global interrupter
	clear_csr(mstatus, MSTATUS_MIE);

	// Initialize interrupter handler
	for (int i = 0; i < PIC_NUM_INTERRUPTS; i++)
		pic_interrupt_handlers[i] = DefaultInterruptHandler;

	// Enable global interrupt
	set_csr(mstatus, MSTATUS_MIE);
}

void hw_business_process(void)
{
	printf("AOCPU image version='%s'\n", CONFIG_COMPILE_TIME);
	OUTPUT_VERSION_FULL_INFO();

	// Initialize GPIOs, PIC and timer
	//vGPIOInit();
	vPICInit();
	stick_mem_init();
	//write watchdog flag
	stick_mem_write(STICK_REBOOT_FLAG, 0xd);

	// Delay
	for (uint32_t i = 0; i < 0xffff; ++i)
		;

	vMbInit();

	vCoreFsmIdleInit();
	vCecCallbackInit(CEC_CHIP_T5);
	write_csr(mtvec, &trap_entry);
	write_csr_mivec(&irq_entry);

	vRtcInit();
	vETHMailboxCallback();
	create_str_task();
	vKeyPadCreate();
}

