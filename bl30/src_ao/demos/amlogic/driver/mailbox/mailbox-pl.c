
/*
 *  Copyright (C) 2014-2018 Amlogic, Inc. All rights reserved.
 *
 *  All information contained herein is Amlogic confidential.
 *
 */

/*Mailbox driver*/
#include <stdint.h>
#include <stdlib.h>
#include <util.h>
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

#include "mailbox.h"
#include "mailbox-pl-in.h"
#include "mailbox-htbl.h"
#include "mailbox-api.h"

#define MBTAG		"AOCPU"
#define PRINT_DBG(...)	//printf(__VA_ARGS__)
#define PRINT_ERR(...)	printf(__VA_ARGS__)
#define PRINT(...)	printf(__VA_ARGS__)

#define AO_MBOX_ONLY_SYNC	1

void *g_tbl_ao;

TaskHandle_t mbHandler;
TaskHandle_t mbAsyncHandler;
static uint32_t ulSyncTaskWake;
mbPackInfo syncMbInfo;

extern void vRpcUserCmdInit(void);

static void vEnterCritical(void)
{
        taskENTER_CRITICAL();
}

static void vExitCritical(void)
{
        taskEXIT_CRITICAL();
}


/*ARM 2 AOCPU mailbox*/
static void vAoRevMbHandler(uint32_t inmbox)
{
	BaseType_t xYieldRequired = pdFALSE;
	uint32_t mbox = inmbox;
	mbPackInfo mbInfo;
	MbStat_t st;
	uint32_t *addr = NULL;
	uint32_t ulMbCmd, ulSize, ulSync;

	PRINT_DBG("[%s]: prvRevMbHandler mbox 0x%x\n", MBTAG, mbox);
	st = xGetMboxStats(MAILBOX_STAT(mbox));
	addr = xRevAddr(xGetChan(mbox));
	ulMbCmd = st.cmd;
	ulSize = st.size;
	ulSync = st.sync;

	PRINT_DBG("[%s]: prvRevMbHandler 0x%x, 0x%x, 0x%x\n", MBTAG, ulMbCmd, ulSize, ulSync);

	if (ulMbCmd == 0) {
		PRINT("mbox cmd is 0, cannot match\n");
		vClrMboxStats(MAILBOX_CLR(mbox));
		return;
	}

	if (ulSize != 0)
		vGetPayload(addr, &mbInfo.mbdata, ulSize);
	else
		PRINT("mbox size is 0,no need to get payload\n");

	PRINT_DBG("%s taskid: 0x%llx\n", MBTAG, mbInfo.mbdata.taskid);
	PRINT_DBG("%s complete: 0x%llx\n", MBTAG, mbInfo.mbdata.complete);
	PRINT_DBG("%s ullclt: 0x%llx\n", MBTAG, mbInfo.mbdata.ullclt);

	switch (ulSync) {
	case MB_SYNC:
		if (ulSyncTaskWake)
			break;
		PRINT_DBG("[%s]: SYNC\n", MBTAG);
		ulSyncTaskWake = 1;
		mbInfo.ulCmd = ulMbCmd;
		mbInfo.ulSize = ulSize;
		mbInfo.ulChan = xGetChan(mbox);
		syncMbInfo = mbInfo;
		vTaskNotifyGiveFromISR(mbHandler, &xYieldRequired);
		portYIELD_FROM_ISR(xYieldRequired);
		break;
	case MB_ASYNC:
#ifdef AO_MBOX_ONLY_SYNC
		PRINT_DBG("[%s]: ASYNC no support\n", MBTAG);
		vClrMboxStats(MAILBOX_CLR(mbox));
#else
		mbInfo.ulCmd = ulMbCmd;
		mbInfo.ulSize = ulSize;
		xQueueSendToBackFromISR(xRevAsyncQueue, (const void *)&mbInfo,
                                        &xYieldRequired);
		vClrMboxStats(MAILBOX_CLR(mbox));
		portYIELD_FROM_ISR(xYieldRequired);
#endif
		break;
	default:
		PRINT_ERR("[%s]: Not SYNC or ASYNC, Fail\n", MBTAG);
		vClrMboxStats(MAILBOX_CLR(mbox));
		break;
	}
}

static void vSyncTask(void *pvParameters)
{
	uint32_t *addr = NULL;
	uint32_t mbox = 0;
	int index = 0;

	pvParameters = pvParameters;
	while (1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		PRINT_DBG("[%s]:MbSyncTas\n", MBTAG);

		index = mailbox_htbl_invokeCmd(g_tbl_ao, syncMbInfo.ulCmd,
					       syncMbInfo.mbdata.data);
		mbox = xGetRevMbox(syncMbInfo.ulChan);
		addr = xSendAddrBack(syncMbInfo.ulChan);
		PRINT_DBG("[%s]:MbSyncTask mbox:%d\n", MBTAG, mbox);
		if (index != 0) {
			if (index == MAX_ENTRY_NUM) {
				memset(&syncMbInfo.mbdata.data, 0, sizeof(syncMbInfo.mbdata.data));
				syncMbInfo.mbdata.status = ACK_FAIL;
				vReBuildPayload(addr, &syncMbInfo.mbdata, sizeof(syncMbInfo.mbdata));
				PRINT_ERR("[%s]: undefine cmd or no callback\n", MBTAG);
			} else {
				PRINT_DBG("[%s]:MbSyncTask re len:%d\n", MBTAG, sizeof(syncMbInfo.mbdata));
				syncMbInfo.mbdata.status = ACK_OK;
				vReBuildPayload(addr, &syncMbInfo.mbdata, sizeof(syncMbInfo.mbdata));
			}
		}

		vEnterCritical();
		PRINT_DBG("[%s]:MbSync clear mbox:0x%lx\n", MBTAG, mbox);
		ulSyncTaskWake = 0;
		vClrMboxStats(MAILBOX_CLR(mbox));
		vExitCritical();
	}
}

static void vMbHandleReeIsr(void);
static void vMbHandleReeIsr(void)
{
	vAoRevMbHandler(MAILBOX_ARMREE2AO);
}

static void vMbHandleTeeIsr(void);
static void vMbHandleTeeIsr(void)
{
	vAoRevMbHandler(MAILBOX_ARMTEE2AO);
}

void vMbInit(void)
{
	PRINT("[%s]: mailbox init start\n", MBTAG);

	mailbox_htbl_init(&g_tbl_ao);

	RegisterIrq(MAILBOX_AOCPU_REE_IRQ, 6, vMbHandleReeIsr);
	RegisterIrq(MAILBOX_AOCPU_TEE_IRQ, 6, vMbHandleTeeIsr);

	EnableIrq(MAILBOX_AOCPU_REE_IRQ);
	EnableIrq(MAILBOX_AOCPU_TEE_IRQ);

	xTaskCreate(vSyncTask,
		    "AoSyncTask",
		    configMINIMAL_STACK_SIZE,
		    0,
		    MHU_MB_TASK_PRIORITIES,
		    (TaskHandle_t *)&mbHandler);

	vRpcUserCmdInit();
	PRINT("[%s]: mailbox init end\n", MBTAG);
}

BaseType_t xInstallRemoteMessageCallbackFeedBack(uint32_t ulChan, uint32_t cmd,
						 void *(handler) (void *),
						 uint8_t needFdBak)
{
	VALID_CHANNEL(ulChan);
	UNUSED(ulChan);
	return mailbox_htbl_reg_feedback(g_tbl_ao, cmd, handler, needFdBak);
}

BaseType_t xUninstallRemoteMessageCallback(uint32_t ulChan, int32_t cmd)
{
	UNUSED(ulChan);
	return mailbox_htbl_unreg(g_tbl_ao, cmd);
}
