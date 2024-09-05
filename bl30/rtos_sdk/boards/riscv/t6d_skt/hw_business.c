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
#include "suspend.h"
#include "vrtc.h"
#include "hwspinlock.h"
#include "ir.h"

void hw_business_process(void)
{
	vMbInit();
	//vCecCallbackInit(CEC_CHIP_T6D);
	vRtcInit();
	//rtc_init();
	vETHMailboxCallback();
	vIRMailboxEnable();
	create_str_task();
#ifdef CONFIG_HWSPINLOCK
	vHwLockInit(HW_SPIN_LOCK0, 0);
#endif
}
