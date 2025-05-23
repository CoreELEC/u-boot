/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "gpio.h"
#include "projdefs.h"
#include "portmacro.h"
#include "string.h"

#include <unistd.h>
#include "n200_func.h"
#include "common.h"
#include "ir.h"
#include "mailbox-api.h"
#include "stick_mem.h"

#include "ir_drv.h"
#include "drv_errno.h"

static struct IRPowerKey prvDefWakeupList[MAX_KEY_NUM];
static uint8_t ucDefWakeupNum;

#define ERR_IR(errno) (DRV_ERRNO_IR_BASE | errno)
#define IR_PRT_ENABLE	0

static uint8_t ucIsDebugEnable;
#define IRDebug(fmt, x...)                                                    \
	do {                                                                  \
		if (ucIsDebugEnable)                                          \
			printf("%s " fmt, DRIVE_NAME, ##x);                  \
	} while (0)

#if (IR_PRT_ENABLE)
#define IRError(fmt, x...) printf("%s " fmt, DRIVE_NAME, ##x)
#else
#define IRError(fmt, x...)
#endif

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
		return ERR_IR(DRV_ERROR_PARAMETER);
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

	IRDebug("mode: 0x%x\n", usWorkMode);
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
	IRDebug("rec: 0x%x\n", xDrvData->ulFrameCode);
	if (!xDrvData->ulFrameCode)
		return;

	/* search power key list */
	for (; ucIndex < xDrvData->ucPowerKeyNum; ucIndex++)
		if (ulPowerKeyList[ucIndex].code == xDrvData->ulFrameCode) {
			xDrvData->ulLastPowerKey = xDrvData->ulFrameCode;
			printf("[ir] key:0x%x\n", xDrvData->ulFrameCode);
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

uint32_t vIRInit(uint16_t usWorkMode, uint16_t usGpio, enum PinMuxType func,
	     struct IRPowerKey *ulPowerKeyList, uint8_t ucPowerKeyNum,
	     void (*vIRHandler)(struct IRPowerKey *pkey))
{
	struct xIRDrvData *xDrvData;

	if (ucIsIRInit()) {
		IRError("all ready init\n");
		return ERR_IR(DRV_ERROR_BUSY);
	}

	if (!IR_INTERRUPT_NUM || !IRQ_NUM_IRIN) {
		IRError("ir irq number not found, ir init failed\n");
		return ERR_IR(DRV_ERROR_IRQ);
	}

	if (!ucDefWakeupNum &&
	    (ulPowerKeyList == NULL || !ucPowerKeyNum)) {
		IRError("not set power key list, ir init failed\n");
		return ERR_IR(DRV_ERROR_SIZE);
	}

	xPinmuxSet(usGpio, func);

	xDrvData = pGetIRDrvData();
	vInitIRWorkMode(usWorkMode);

	if (ucDefWakeupNum) {
		xDrvData->ulPowerKeyList = prvDefWakeupList;
		xDrvData->ucPowerKeyNum = ucDefWakeupNum;
	} else {
		xDrvData->ulPowerKeyList = ulPowerKeyList;
		xDrvData->ucPowerKeyNum = ucPowerKeyNum;
	}
	xDrvData->ulLastPowerKey = 0;
	xDrvData->ucCurWorkMode = usWorkMode;
	xDrvData->vIRHandler = vIRHandler;

	RegisterIrq(IRQ_NUM_IRIN, 2, vIRIntteruptHandler);
	EnableIrq(IRQ_NUM_IRIN);
#if defined(IRQ_NUM_IRIN_EXT)
	RegisterIrq(IRQ_NUM_IRIN_EXT, 2, vIRIntteruptHandler);
	EnableIrq(IRQ_NUM_IRIN_EXT);
#endif

	xDrvData->ucIsInit = 1;

	return 0;
}

void vIRDeint(void)
{
	struct xIRDrvData *xDrvData;

	if (!ucIsIRInit())
		return;

	xDrvData = pGetIRDrvData();

	xDrvData->ucIsInit = 0;
	xDrvData->vIRHandler = NULL;

	DisableIrq(IRQ_NUM_IRIN);
	UnRegisterIrq(IRQ_NUM_IRIN);
#if defined(IRQ_NUM_IRIN_EXT)
	DisableIrq(IRQ_NUM_IRIN_EXT);
	UnRegisterIrq(IRQ_NUM_IRIN_EXT);
#endif
}

static void *prvIRGetInfo(void *msg)
{
	uint8_t i;
	uint32_t cmd = *(uint32_t *)msg;
	uint32_t *data = (uint32_t *)msg + 1;
	struct xIRDrvData *xDrvData = pGetIRDrvData();

	switch (cmd) {
	case IR_MBOX_CMD_SET_DEBUG_LOG:
		ucIsDebugEnable = data[0];
		break;
	case IR_MBOX_CMD_SET_WAKEUP_LIST:
		ucDefWakeupNum = data[0];
		for (i = 0; i < ucDefWakeupNum; i++) {
			prvDefWakeupList[i].type = (data[1] >> i) & 0x1;
			prvDefWakeupList[i].code = data[i + 2];
		}
		break;
	case IR_MBOX_CMD_GET_WAKEUP_KEY:
		memset(msg, 0, MBOX_BUF_LEN);
#ifdef CONFIG_STICK_MEM
		stick_mem_read(STICK_IR_WAKEUP_KEY, msg);
		stick_mem_write(STICK_IR_WAKEUP_KEY, 0);
#endif
		break;
	case IR_MBOX_CMD_SET_STATUS:
		if (data[0])
			vIRInit(MODE_HARD_NEC, 0, PIN_FUNC_INVALID, NULL, 0,
				NULL);
		else
			vIRDeint();
		break;
	case IR_MBOX_CMD_GET_PREBOOT_KEY:
		memset(msg, 0, MBOX_BUF_LEN);
		*(uint32_t *)msg = xDrvData->ulLastPowerKey;
		break;
	default:
		break;
	}

	return NULL;
}

uint32_t vIRMailboxEnable(void)
{
	int32_t ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL,
						    MBX_CMD_GET_IR_INFO,
						    prvIRGetInfo, 1);
	if (ret == MBOX_CALL_MAX) {
		return ERR_IR(DRV_ERROR_UNSUPPORTED);
	}
	return 0;
}

void vIR32KInit(uint32_t ulFrame0, uint32_t ulFrame1)
{
#ifdef RTCCTRL_REG_CTRL0
	REG32(RTCCTRL_REG_CTRL3) = ulFrame0;
	REG32(RTCCTRL_REG_CTRL4) = ulFrame1;

	if (ulFrame0)
		REG32(RTCCTRL_REG_CTRL5) = 0xffffffff;
	else
		REG32(RTCCTRL_REG_CTRL5) = 0x00;

	if (ulFrame1)
		REG32(RTCCTRL_REG_CTRL6) = 0xffffffff;
	else
		REG32(RTCCTRL_REG_CTRL6) = 0x0;

	REG32(RTCCTRL_REG_CTRL9) = (1 << 12);
	REG32(RTCCTRL_REG_CTRL9) = (1 << 4);
#endif
}
