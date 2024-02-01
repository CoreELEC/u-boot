/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>

#include <unistd.h>
#include "n200_func.h"
#include "uart.h"
#include "common.h"
#include "riscv_encoding.h"

#include "mailbox-api.h"

#define MBTAG "AOCPU"
#define PRINT_DBG(...) printf(__VA_ARGS__)
#define PRINT_ERR(...) printf(__VA_ARGS__)
#define PRINT(...) printf(__VA_ARGS__)

struct Uintcase {
	char data[20];
	uint32_t ulTaskDelay;
};

static inline void *mbmemset(void *dst, int val, size_t count)
{
	char *ptr = dst;

	while (count--)
		*ptr++ = val;

	return dst;
}

static inline void *mbmemcpy(void *dst, const void *src, size_t len)
{
	const char *s = src;
	char *d = dst;

	while (len--)
		*d++ = *s++;

	return dst;
}

static void xMboxUintReeTestCase(void *msg)
{
	struct Uintcase *pdata = msg;
	char back[20] = "Response AOCPU\n";

	PRINT("[%s]: scpi %s\n", MBTAG, pdata->data);
	mbmemset(msg, 0, MBOX_BUF_LEN);
	mbmemcpy(msg, back, sizeof(back));

	PRINT("[%s]: delay after %ld\n", MBTAG, pdata->ulTaskDelay);
}

static void xMboxUintTeeTestCase(void *msg)
{
	char *s = msg;

	PRINT("[%s]: from tee: %s\n", MBTAG, s);
}

static int vRegisterRpcCallBack(void)
{
	int ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_RPCUINTREE_TEST,
						    (void *)xMboxUintReeTestCase, 1);
	if (ret) {
		PRINT("[%s]: mbox cmd 0x%x register fail\n", MBTAG, MBX_CMD_RPCUINTREE_TEST);
		return ERR_MBOX(ENOSPC);
	}

	ret = xInstallRemoteMessageCallbackFeedBack(AOTEE_CHANNEL, MBX_CMD_RPCUINTTEE_TEST,
						    (void *)xMboxUintTeeTestCase, 0);
	if (ret) {
		PRINT("[%s]: mbox cmd 0x%x register fail\n", MBTAG, MBX_CMD_RPCUINTTEE_TEST);
		return ERR_MBOX(ENOSPC);
	}

	return 0;
}

int vRpcUserCmdInit(void)
{
	return vRegisterRpcCallBack();
}
