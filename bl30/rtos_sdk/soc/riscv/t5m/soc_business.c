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
}

