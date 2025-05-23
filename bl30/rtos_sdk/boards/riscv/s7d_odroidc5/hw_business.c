/*
 * Copyright (c) 2025 Hardkernel Co., Ltd. All rights reserved.
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
#include "ir.h"
#include "keypad.h"

void hw_business_process(void)
{
	iUartInit();
	vMbInit();
	vCecCallbackInit(CEC_CHIP_S7D);
	vRtcInit();
	vDynamicKeypadInit();
	vETHMailboxCallback();
	vIRMailboxEnable();
	create_str_task();
}
