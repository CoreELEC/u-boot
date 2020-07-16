
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
#include "myprintf.h"
#include <unistd.h>
#include "n200_eclic.h"
#include "n200_func.h"
#include "uart.h"
#include "common.h"
#include "riscv_encoding.h"

#include "mailbox.h"
#include "mailbox-irq.h"
#include "mailbox-in.h"
#include "mailbox-htbl.h"
#include "mailbox-api.h"

#define TAG "AOCPU"
#define PRINT_DBG	//printf
#define PRINT_ERR	printf
#define PRINT		printf

#define MHU_MB_STK_SIZE		2048
#define MB_DATA_SHR_SIZE	240
#define TASK_PRIORITY		0x2

#define AO_MBOX_ONLY_SYNC	1

void *g_tbl_ao;

TaskHandle_t mbHandler;
TaskHandle_t mbAsyncHandler;
static uint32_t ulSyncTaskWake;
mbPackInfo syncMbInfo;

extern xHandlerTableEntry xMbHandlerTable[IRQ_MAX];
extern void vRpcUserCmdInit(void);

void vEnterCritical(UBaseType_t *uxIsr)
{
        taskENTER_CRITICAL();
        *uxIsr = 0;
}

void vExitCritical(UBaseType_t uxSaveIsr)
{
        taskEXIT_CRITICAL();
        uxSaveIsr = 0;
}

void vMbHandleIsr(void)
{
	unsigned int val = 0;
	unsigned int ulPreVal = 0;
	unsigned int ulIrqMask = IRQ_MASK;
	int i = 0;

	val = xGetMbIrqStats();
	PRINT_DBG("[%s]: mb isr: 0x%x\n", TAG, val);
	val &= ulIrqMask;
	while (val) {
		for (i = 0; i <= IRQ_MAX; i++) {
			if (val & (1 << i)) {
				if (xMbHandlerTable[i].vHandler != NULL) {
					xMbHandlerTable[i].vHandler(xMbHandlerTable[i].vArg);
				}
			}
		}
		ulPreVal = val;
		val = xGetMbIrqStats();
		val &= ulIrqMask;
		val = (val | ulPreVal) ^ ulPreVal;
	}
}
DECLARE_IRQ(IRQ_NUM_MB_4, vMbHandleIsr)

/*Ree 2 AOCPU mailbox*/
static void vAoRevMbHandler(void *vArg)
{
	BaseType_t xYieldRequired = pdFALSE;
	UBaseType_t uxSaveIsr;
	uint32_t mbox = vArg;
	mbPackInfo mbInfo;
	MbStat_t st;
	uint32_t addr, ulMbCmd, ulSize, ulSync;

	st = xGetMboxStats(MAILBOX_STAT(mbox));
	addr = xDspRevAddrMbox(mbox);
	ulMbCmd = st.cmd;
	ulSize = st.size;
	ulSync = st.sync;

	PRINT_DBG("[%s]: prvDspRevMbHandler 0x%x, 0x%x, 0x%x\n", TAG, ulMbCmd, ulSize, ulSync);

	if (ulMbCmd == 0) {
		PRINT("[%s] mbox cmd is 0, cannot match\n");
		vClrMboxStats(MAILBOX_CLR(mbox));
		vClrMbInterrupt(IRQ_REV_BIT(mbox));
		return;
	}

	if (ulSize != 0)
		vGetPayload(addr, &mbInfo.mbdata, ulSize);
	else
		PRINT("[%s] mbox size is 0,no need to get payload\n");

	PRINT_DBG("%s taskid: 0x%llx\n", TAG, mbInfo.mbdata.taskid);
	PRINT_DBG("%s complete: 0x%llx\n", TAG, mbInfo.mbdata.complete);
	PRINT_DBG("%s ullclt: 0x%llx\n", TAG, mbInfo.mbdata.ullclt);

	switch (ulSync) {
	case MB_SYNC:
		if (ulSyncTaskWake)
			break;
		PRINT_DBG("[%s]: SYNC\n", TAG);
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
		PRINT_DBG("[%s]: ASYNC no support\n", TAG);
		vClrMboxStats(MAILBOX_CLR(mbox));
		vClrMbInterrupt(IRQ_REV_BIT(mbox));
#else
		mbInfo.ulCmd = ulMbCmd;
		mbInfo.ulSize = ulSize;
		 xQueueSendToBackFromISR(xRevAsyncQueue, (const void *)&mbInfo,
                                        &xYieldRequired);
		vClrMboxStats(MAILBOX_CLR(mbox));
		vClrMbInterrupt(IRQ_REV_BIT(mbox));
		portYIELD_FROM_ISR(xYieldRequired);
#endif
		break;
	default:
		break;
	}
}

void vSyncTask(void *pvParameters)
{
	uint32_t rev = pvParameters;
	uint32_t addr = 0;
	uint32_t mbox = 0;
	UBaseType_t uxSaveIsr;
	int index = 0;

	pvParameters = pvParameters;
	while (1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		PRINT_DBG("[%s]:ReeSyncTask mbox:%d\n", TAG, mbox);

		index = mailbox_htbl_invokeCmd(g_tbl_ao, syncMbInfo.ulCmd,
					       syncMbInfo.mbdata.data);
		mbox = xGetRevMbox(syncMbInfo.ulChan);
		addr = xDspSendAddrMbox(mbox);
		if (index != 0) {
			if (index == MAX_ENTRY_NUM) {
				memset(&syncMbInfo.mbdata.data, 0, sizeof(syncMbInfo.mbdata.data));
				syncMbInfo.mbdata.status = ACK_FAIL;
				vBuildPayload(addr, &syncMbInfo.mbdata, sizeof(syncMbInfo.mbdata));
				PRINT_ERR("[%s]: undefine cmd or no callback\n", TAG);
			} else {
				PRINT_DBG("[%s]:ReeSyncTask re len:%d\n", TAG, sizeof(syncMbInfo.mbdata));
				syncMbInfo.mbdata.status = ACK_OK;
				vBuildPayload(addr, &syncMbInfo.mbdata, sizeof(syncMbInfo.mbdata));
			}
		}

		vEnterCritical(&uxSaveIsr);
		PRINT_DBG("[%s]:Ree Sync clear mbox:%d\n", TAG, mbox);
		ulSyncTaskWake = 0;
		vClrMboxStats(MAILBOX_CLR(mbox));
		vClrMbInterrupt(IRQ_REV_BIT(mbox));
		vExitCritical(uxSaveIsr);
	}
}

void vMbInit(void)
{
	int i = 0;
	char name[15];
	uint32_t mbox;

	PRINT("[%s]: mailbox init start\n", TAG);
	mailbox_htbl_init(&g_tbl_ao);

	/* Set MBOX IRQ Handler and Priority */
	vSetMbIrqHandler(IRQ_REV_NUM(MAILBOX_ARMREE2AO), vAoRevMbHandler, MAILBOX_ARMREE2AO, 10);

	vSetMbIrqHandler(IRQ_REV_NUM(MAILBOX_ARMTEE2AO), vAoRevMbHandler, MAILBOX_ARMTEE2AO, 10);

	vEnableIrq(IRQ_NUM_MB_4, 249);

	xTaskCreate(vSyncTask,
		    "AOReeSyncTask",
		    configMINIMAL_STACK_SIZE,
		    0,
		    TASK_PRIORITY,
		    (TaskHandle_t *)&mbHandler);

	vRpcUserCmdInit();
	PRINT("[%s]: mailbox init end\n", TAG);
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
