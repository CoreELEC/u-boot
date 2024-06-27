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
#include "n200_pic_tmr.h"
#include "n200_func.h"
#include "riscv_encoding.h"
#include "timer_source.h"
#include "stick_mem.h"
#include "soc_business.h"

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

void soc_business_process(void)
{
	write_csr(mtvec, &trap_entry);
	write_csr_mivec(&irq_entry);
	// Initialize GPIOs, PIC and timer
	//vGPIOInit();
	vPICInit();
	stick_mem_init();
	stick_mem_write(STICK_REBOOT_FLAG, WATCHDOG_REBOOT);
}

