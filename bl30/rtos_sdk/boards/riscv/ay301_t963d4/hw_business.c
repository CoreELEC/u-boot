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
#if CONFIG_SPI
#include "spi.h"
static TaskHandle_t SpiMasterTaskHandler;
#define SpiMasterTaskPriority 4
#endif

void hw_business_process(void)
{
	iUartInit();
	vMbInit();
	vCecCallbackInit(CEC_CHIP_T5M);
	vRtcInit();
	//rtc_init();
	vETHMailboxCallback();
	vIRMailboxEnable();
	create_str_task();
	vHwLockInit(HW_SPIN_LOCK0, 0);
#if CONFIG_SPI
	vSpicc1Init();
	vMbSpiInit();
	xTaskCreate(vSpiMasterTask, "SpiMaster", configMINIMAL_STACK_SIZE, NULL,
		    SpiMasterTaskPriority, &SpiMasterTaskHandler);
#endif

}
