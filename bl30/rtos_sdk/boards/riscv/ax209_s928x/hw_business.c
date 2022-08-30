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
#include "common.h"
#include "riscv_encoding.h"
#include "mailbox-api.h"
#include "hdmi_cec.h"
#include "stick_mem.h"
#include "sdk_ver.h"
#include "suspend.h"
#include "vrtc.h"
#include "hwspinlock.h"

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

void config_eclic_irqs(void)
{
	eclic_init(ECLIC_NUM_INTERRUPTS);
	eclic_set_nlbits(0);
}

void hw_business_process(void)
{
	uint8_t i = 0;

	printf("AOCPU image version='%s'\n", CONFIG_COMPILE_TIME);
	config_eclic_irqs();
	for (i = 0; i < 4; ++i)
		printf("AOCPU_IRQ_SEL=0x%x\n", REG32(AOCPU_IRQ_SEL0 + i * 4));
	vMbInit();
	vCecCallbackInit(CEC_CHIP_S5);
	vRtcInit();
	create_str_task();
	vHwLockInit(HW_SPIN_LOCK0, 0);
}
