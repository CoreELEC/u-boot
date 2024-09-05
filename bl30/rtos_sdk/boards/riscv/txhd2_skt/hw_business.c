/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "hw_business.h"
#include "uart.h"
#include "eth.h"
#include "common.h"
#include "mailbox-api.h"
#include "hdmi_cec.h"
#include "keypad.h"
#include "fsm.h"
#include "ir.h"

#define RTOS_BOOT_SUCC_REG		AO_DEBUG_REG2

void hw_business_process(void)
{
	vMbInit();
	vCoreFsmIdleInit();
	vCecCallbackInit(CEC_CHIP_TXHD2);
	vRtcInit();
	vETHMailboxCallback();
	vIRMailboxEnable();
	create_str_task();
	vKeyPadCreate();
}

void aocpu_bringup_finished(void)
{
	#define RTOS_RUN_SUCC                  (1 << 0)
	*(volatile uint32_t *)RTOS_BOOT_SUCC_REG |= RTOS_RUN_SUCC;
}

