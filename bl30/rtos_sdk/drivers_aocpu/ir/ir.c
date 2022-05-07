/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "gpio.h"
#include "projdefs.h"
#include "portmacro.h"

#include <unistd.h>
#include "n200_func.h"
#include "common.h"
#include "ir.h"
#include "mailbox-api.h"

#include "ir_drv.h"

static uint8_t ucIsDebugEnable;
static struct IRPowerKey prvKeyCodeList[MAX_KEY_NUM] = {};
static uint32_t key_cnt;

#define IRDebug(fmt, x...)                                                                         \
	do {                                                                                       \
		if (ucIsDebugEnable)                                                               \
			printf("%sDebug: %s: " fmt, DRIVE_NAME, __func__, ##x);                    \
	} while (0)

#define IRError(fmt, x...) printf("%sError: %s: " fmt, DRIVE_NAME, __func__, ##x)

static inline void prvIRRegWrite(uint8_t ucCTL, uint8_t ucAddr, uint32_t ulval)
{
	uint32_t ulRegBase = 0;

	if (!IR_BASE_ADDR || !IR_BASE_ADDR_OLD) {
		IRError("IR_BASE_ADDR not found!\n");
		return;
	}

	ulRegBase = IS_LEGACY_CTRL(ucCTL);

	REG32((unsigned long)(ulRegBase + ucAddr)) = ulval;
}

static inline uint32_t prvIRRegRead(uint8_t ucCTL, uint8_t ucAddr)
{
	uint32_t ulRegBase = 0;

	if (!IR_BASE_ADDR || !IR_BASE_ADDR_OLD) {
		IRError("IR_BASE_ADDR not found!\n");
		return -1;
	}

	ulRegBase = IS_LEGACY_CTRL(ucCTL);

	return REG32(ulRegBase + ucAddr);
}

static inline void prvIRRegUpdateBit(uint8_t ucCTL, uint8_t ucAddr, uint32_t ulMask, uint32_t ulVal)
{
	uint32_t ulRegBase = 0;

	if (!IR_BASE_ADDR || !IR_BASE_ADDR_OLD) {
		IRError("IR_BASE_ADDR not found!\n");
		return;
	}

	ulRegBase = IS_LEGACY_CTRL(ucCTL);

	REG32_UPDATE_BITS(ulRegBase + ucAddr, ulMask, ulVal);
}

static void vSetIRWorkMode(uint16_t usWorkMode, uint8_t ucWorkCTL)
{
	uint8_t ucRegSize = 0;
	uint8_t i = 0;
	uint32_t ulValue = 0;
	const struct xRegProtocolMethod **xProData;

	xProData = pGetSupportProtocol();

	for (; (*xProData) != NULL;) {
		if ((*xProData)->ucProtocol == usWorkMode)
			break;
		xProData++;
	}

	if (!*xProData) {
		IRError("Not support this type Protocol!\n");
		return;
	}

	/*clear status and disable ir decoder */
	prvIRRegRead(ucWorkCTL, REG_FRAME);
	prvIRRegWrite(ucWorkCTL, REG_REG1, 0x01);

	ucRegSize = (*xProData)->ucRegNum;

	for (; i < ucRegSize; i++)
		prvIRRegWrite(ucWorkCTL, (*xProData)->RegList[i].ucReg,
			      (*xProData)->RegList[i].ulVal);

	/*reset IR decoder when reinstall */
	ulValue = prvIRRegRead(ucWorkCTL, REG_REG1);
	ulValue |= 1;
	prvIRRegWrite(ucWorkCTL, REG_REG1, ulValue);
	ulValue &= ~0x01;
	prvIRRegWrite(ucWorkCTL, REG_REG1, ulValue);
}

void vInitIRWorkMode(uint16_t usWorkMode)
{
	if (ENABLE_LEGACY_CTL(usWorkMode))
		vSetIRWorkMode(LEGACY_IR_CTL_MASK(usWorkMode), LEGACY_CTL);
	else
		prvIRRegWrite(LEGACY_CTL, REG_REG1, 0);

	IRDebug("change mode to 0x%x\n", usWorkMode);
	if (MULTI_IR_CTL_MASK(usWorkMode))
		vSetIRWorkMode(MULTI_IR_CTL_MASK(usWorkMode), MULTI_CTL);
}

static void prvCheckPowerKey(void)
{
	struct xIRDrvData *xDrvData;
	struct IRPowerKey *ulPowerKeyList;
	uint8_t ucIndex = 0;

	xDrvData = pGetIRDrvData();
	ulPowerKeyList = xDrvData->ulPowerKeyList;
	if (!xDrvData->ulFrameCode) {
		IRDebug("invalid key or repeat :0x%x\n", xDrvData->ulFrameCode);
		return;
	};

	IRDebug("receive key code :0x%x\n", xDrvData->ulFrameCode);
	/* search power key list */
	for (; ucIndex < xDrvData->ucPowerKeyNum; ucIndex++)
		if (ulPowerKeyList[ucIndex].code == xDrvData->ulFrameCode) {
			printf("receive the right power key:0x%x\n", xDrvData->ulFrameCode);
			if (xDrvData->vIRHandler)
				xDrvData->vIRHandler(&ulPowerKeyList[ucIndex]);
		}
}

static void vIRIntteruptHandler(void)
{
	uint32_t ulDecodeStatus = 0;
	uint8_t ucIndex = 0;
	uint16_t ucCurWorkMode;
	struct xIRDrvData *xDrvData;

	xDrvData = pGetIRDrvData();
	ucCurWorkMode = xDrvData->ucCurWorkMode;

	/*choose controller */
	for (; ucIndex < (ENABLE_LEGACY_CTL(ucCurWorkMode) ? 2 : 1); ucIndex++) {
		ulDecodeStatus = prvIRRegRead(ucIndex, REG_STATUS);
		if (ulDecodeStatus & 0x08) {
			xDrvData->ucWorkCTL = ucIndex;
			break;
		}
	}

	if (ucIndex == ERROR_CTL) {
		IRError("error interrupt !\n");
		return;
	}

	ulDecodeStatus = prvIRRegRead(xDrvData->ucWorkCTL, REG_STATUS);

	xDrvData->ucIRStatus = STATUS_NORMAL;
	if (ulDecodeStatus & 0x01)
		xDrvData->ucIRStatus = STATUS_REPEAT;

	xDrvData->ulFrameCode = prvIRRegRead(xDrvData->ucWorkCTL, REG_FRAME);

	prvCheckPowerKey();
}

int8_t ucIsIRInit(void)
{
	struct xIRDrvData *xDrvData;

	xDrvData = pGetIRDrvData();
	return xDrvData->ucIsInit;
}

void vIRInit(uint16_t usWorkMode, uint16_t usGpio, enum PinMuxType func,
	     struct IRPowerKey *ulPowerKeyList, uint8_t ucPowerKeyNum,
	     void (*vIRHandler)(struct IRPowerKey *pkey))
{
	struct xIRDrvData *xDrvData;

	if (ucIsIRInit()) {
		IRError("all ready init\n");
		return;
	}

	if (!IR_INTERRUPT_NUM || !IRQ_NUM_IRIN) {
		IRError("ir irq number not found, ir init failed\n");
		return;
	}

	if (ulPowerKeyList == NULL || !ucPowerKeyNum) {
		IRError("not set power key list, ir init failed\n");
		return;
	}

	if (xPinmuxSet(usGpio, func)) {
		IRError("pin mux setting error\n");
		return;
	}

	xDrvData = pGetIRDrvData();
	vInitIRWorkMode(usWorkMode);

	xDrvData->ulPowerKeyList = ulPowerKeyList;
	xDrvData->ucPowerKeyNum = ucPowerKeyNum;
	xDrvData->ucCurWorkMode = usWorkMode;
	xDrvData->vIRHandler = vIRHandler;

	RegisterIrq(IRQ_NUM_IRIN, 2, vIRIntteruptHandler);
	EnableIrq(IRQ_NUM_IRIN);

	xDrvData->ucIsInit = 1;
}

void vIRDeint(void)
{
	struct xIRDrvData *xDrvData;

	xDrvData = pGetIRDrvData();

	xDrvData->ucIsInit = 0;
	xDrvData->vIRHandler = NULL;

	DisableIrq(IRQ_NUM_IRIN);
	UnRegisterIrq(IRQ_NUM_IRIN);
}

void vIRGetKeyCode(struct IRPowerKey *PowerKeyList)
{
	struct IRPowerKey *Keydest = PowerKeyList;
	struct IRPowerKey *KeyList = prvKeyCodeList;

	while (key_cnt--)
		*Keydest++ = *KeyList++;
}

static void *prvIRGetInfo(void *msg)
{
	uint32_t key_num, i;
	uint32_t *key_code, *key_type;

	key_num = *(u32 *)msg;
	key_code = ((u32 *)msg) + 1;
	key_type = ((u32 *)msg) + key_num / 2 + 1;

	for (i = 0; i < key_num / 2; i++) {
		prvKeyCodeList[i].code = *key_code;
		prvKeyCodeList[i].type = *key_type;
		key_code++;
		key_type++;
	}

	key_cnt = i;
	return NULL;
}

void vIRMailboxEnable(void)
{
	int32_t ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_GET_IR_INFO,
						    prvIRGetInfo, 1);
	if (ret == MBOX_CALL_MAX) {
		printf("mailbox cmd 0x%x register fail\n");
		return;
	}
}
