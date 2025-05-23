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
#include "hw_business.h"
#include "ir.h"

void hw_business_process(void)
{
	iUartInit();
	vMbInit();
	//	vCecCallbackInit(CEC_CHIP_SC2);
	vRtcInit();
	//rtc_init();
	vETHMailboxCallback();
	vIRMailboxEnable();
	create_str_task();
}
