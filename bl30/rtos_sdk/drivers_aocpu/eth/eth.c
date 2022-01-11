/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "suspend.h"
#include "interrupt.h"
#include "eth.h"

uint32_t ethIrq;
void eth_handler(void)
{
	uint32_t buf[4] = {0};

	buf[0] = ETH_PMT_WAKEUP;
	STR_Wakeup_src_Queue_Send_FromISR(buf);
	DisableIrq(ethIrq);
}

void vETHInit(uint32_t ulIrq,function_ptr_t handler)
{
	ethIrq = ulIrq;
	RegisterIrq(ulIrq, 2, handler);
	EnableIrq(ulIrq);
}

void vETHDeint(void)
{
	DisableIrq(ethIrq);
	UnRegisterIrq(ethIrq);
}

int eth_deinit = 0;
void eth_handler_t5(void)
{
	uint32_t buf[4] = {0};
	if (eth_deinit == 0) {
		buf[0] = ETH_PMT_WAKEUP;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		DisableIrq(ethIrq);
	} else {
		eth_deinit = 0;
	}
}

void vETHDeint_t5(void)
{
	eth_deinit = 1;
	DisableIrq(ethIrq);
	UnRegisterIrq(ethIrq);
}