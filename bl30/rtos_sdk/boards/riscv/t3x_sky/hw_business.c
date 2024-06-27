/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "common.h"
#include "uart.h"
#include "eth.h"
#include "mailbox-api.h"
#include "hdmi_cec.h"
#include "suspend.h"
#include "vrtc.h"
#include "hwspinlock.h"
#include "hw_business.h"

void hw_business_process(void)
{
	vMbInit();
	vCecCallbackInit(CEC_CHIP_T3X);
	vRtcInit();
	//rtc_init();
	vETHMailboxCallback();
	create_str_task();
	vHwLockInit(HW_SPIN_LOCK0, 0);
}
