
/*
 *  Copyright (C) 2014-2018 Amlogic, Inc. All rights reserved.
 *
 *  All information contained herein is Amlogic confidential.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <util.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include "myprintf.h"

#include <unistd.h>
#include "n200_eclic.h"
#include "n200_func.h"
#include "uart.h"
#include "common.h"
#include "riscv_encoding.h"

#include "mailbox-api.h"

#define TAG "AOCPU"
#define PRINT_DBG	//printf
#define PRINT_ERR	printf
#define PRINT		printf


struct Uintcase {
	char data[20];
	uint32_t ulTaskDelay;
};

void xMboxUintReeTestCase(void *msg)
{
	struct Uintcase *pdata = msg;
	char back[20] = "Response AOCPU\n";

	PRINT("[%s]: scpi %s\n", TAG, pdata->data);
	memset(msg, 0, MBOX_BUF_LEN);
	memcpy(msg, back, sizeof(back));

//	vTaskDelay(pdata->ulTaskDelay);
	PRINT("[%s]: delay after %d\n", TAG, pdata->ulTaskDelay);

}

void xMboxUintTeeTestCase(void *msg)
{
	char *s = msg;

	PRINT("[%s]: from tee: %s\n", TAG, s);

}

void vRegisterRpcCallBack(void)
{
	xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_RPCUINTREE_TEST,
					      xMboxUintReeTestCase, 1);
	xInstallRemoteMessageCallbackFeedBack(AOTEE_CHANNEL, MBX_CMD_RPCUINTTEE_TEST,
					      xMboxUintTeeTestCase, 0);
}

void vRpcUserCmdInit(void)
{
        vRegisterRpcCallBack();
}

