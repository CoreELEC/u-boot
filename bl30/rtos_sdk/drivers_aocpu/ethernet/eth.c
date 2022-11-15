/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "suspend.h"
#include "interrupt.h"
#include "mailbox-api.h"
#include "irq.h"
#include "eth.h"

uint32_t ethIrq = IRQ_ETH_PMT_NUM;
uint32_t Serial_T5;
int eth_deinit;
uint32_t eth_wol_flag;
void eth_handler(void)
{
	uint32_t buf[4] = { 0 };

	buf[0] = ETH_PMT_WAKEUP;
	STR_Wakeup_src_Queue_Send_FromISR(buf);
	DisableIrq(ethIrq);
}

void eth_handler_t5(void)
{
	uint32_t buf[4] = { 0 };

	if (eth_deinit == 0) {
		buf[0] = ETH_PMT_WAKEUP;
		STR_Wakeup_src_Queue_Send_FromISR(buf);
		DisableIrq(ethIrq);
	} else {
		eth_deinit = 0;
	}
}

/*type: 0   normal
 *		1   t5 serial
 */
void vETHInit(uint32_t type)
{
	if (!eth_wol_flag)
		return;
	printf("%s type=%d\n", __func__, type);
	Serial_T5 = type;
	if (type == 1)
		RegisterIrq(ethIrq, 2, eth_handler_t5);
	else
		RegisterIrq(ethIrq, 2, eth_handler);
}

void vETHDeint(void)
{
	if (!eth_wol_flag)
		return;
	printf("%s\n", __func__);
	if (Serial_T5)
		eth_deinit = 1;

	DisableIrq(ethIrq);
	UnRegisterIrq(ethIrq);
}

void vETHEnableIrq(void)
{
	if (!eth_wol_flag)
		return;
	printf("vETH enable irq\n");
	EnableIrq(ethIrq);
}

int get_ETHWol_flag(void)
{
	return eth_wol_flag;
}

static void *prvETHSetWol(void *msg)
{
	eth_wol_flag = *(uint32_t *)msg;
	printf("vETH wol flag = %d\n", eth_wol_flag);
	return NULL;
}

void vETHMailboxCallback(void)
{
	int32_t ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_SET_ETHERNET_WOL,
		prvETHSetWol, 1);
	if (ret == MBOX_CALL_MAX) {
		printf("mailbox cmd 0x%x register fail\n");
		return;
	}
}
