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
#include "common.h"
#include "mailbox-api.h"
#include "hdmi_cec.h"
#include "stick_mem.h"
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

/* Interrupt handler */
void GPIOInterruptHandler(uint32_t num, uint32_t priority);
void vApplicationIdleHook( void );


extern void trap_entry(void);
extern void irq_entry(void);


/* Binary Semaphore */
QueueHandle_t xGPIOSemaphore[INT_TEST_NEST_DEPTH];
QueueHandle_t xMessageQueue[TASK_TEST_QUEUE_NUM];

/*
static void vPrintString(const char* msg)
{
	taskENTER_CRITICAL();

	taskEXIT_CRITICAL();
}
*/
/* function: vPICInit */
static void vPICInit(void) {

	// Disable global interrupter
	clear_csr(mstatus, MSTATUS_MIE);

	// Initialize interrupter handler
	for (int i = 0; i < PIC_NUM_INTERRUPTS; i ++) {
		pic_interrupt_handlers[i] = DefaultInterruptHandler;
	}
#if 0
#if !(SIGNAL_BOARD_ENABLE)

   // Enable GPIO interrupter
	enable_interrupt(GPIO_INT_SOURCE(8),  1, GPIOInterruptHandler8);
	enable_interrupt(GPIO_INT_SOURCE(9),  2, GPIOInterruptHandler9);
	enable_interrupt(GPIO_INT_SOURCE(10),  3, GPIOInterruptHandler10);
	enable_interrupt(GPIO_INT_SOURCE(11),  4, GPIOInterruptHandler11);
	enable_interrupt(GPIO_INT_SOURCE(12),  5, GPIOInterruptHandler12);
	enable_interrupt(GPIO_INT_SOURCE(13),  6, GPIOInterruptHandler13);
	//enable_interrupt(GPIO_INT_SOURCE(14),  7, GPIOInterruptHandler14);
	//enable_interrupt(GPIO_INT_SOURCE(15),  7, GPIOInterruptHandler15);

#endif
#endif
	// Enable global interrupt
	set_csr(mstatus, MSTATUS_MIE);
}

extern void vMbInit(void);
extern void vCoreFsmIdleInit(void);
extern void vRtcInit(void);
extern void create_str_task(void);

void hw_business_process(void)
{
	printf("AOCPU image version='%s'\n", CONFIG_COMPILE_TIME);

	// Initialize GPIOs, PIC and timer
	//vGPIOInit();
	vPICInit();
	stick_mem_init();
	//write watchdog flag
	stick_mem_write(STICK_REBOOT_FLAG, 0xd);

	// Delay
	for (uint32_t i = 0; i < 0xffff; ++i);

	vMbInit();

	vCoreFsmIdleInit();
	vCecCallbackInit(CEC_CHIP_T5);
	write_csr(mtvec, &trap_entry);
	write_csr_mivec(&irq_entry);

	vRtcInit();
	create_str_task();
}

