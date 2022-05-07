/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _IR_DRIVER_H_
#define _IR_DRIVER_H_
#include <ir.h>
#include "register.h"
#include "irq.h"

#define DRIVE_NAME "[ir]"

/* only for sc2 now */
#define IR_INTERRUPT_NUM 22

#ifndef IR_INTERRUPT_NUM
#define IR_INTERRUPT_NUM 0
#endif

#ifndef IRQ_NUM_IRIN
#define IRQ_NUM_IRIN 0
#endif

#ifdef IRCTRL_IR_DEC_LDR_ACTIVE
#define IR_BASE_ADDR_OLD IRCTRL_IR_DEC_LDR_ACTIVE
#else
#ifdef AO_IR_DEC_LDR_ACTIVE
#define IR_BASE_ADDR_OLD AO_IR_DEC_LDR_ACTIVE
#else
#define IR_BASE_ADDR_OLD 0
#endif
#endif

#ifdef IRCTRL_MF_IR_DEC_LDR_ACTIVE
#define IR_BASE_ADDR IRCTRL_MF_IR_DEC_LDR_ACTIVE
#else
#ifdef AO_MF_IR_DEC_LDR_ACTIVE
#define IR_BASE_ADDR AO_MF_IR_DEC_LDR_ACTIVE
#else
#define IR_BASE_ADDR 0
#endif
#endif

#define IS_LEGACY_CTRL(x) ((x) ? (IR_BASE_ADDR_OLD) : (IR_BASE_ADDR))
#define ENABLE_LEGACY_CTL(x) (((x) >> 8) & 0xff)
#define LEGACY_IR_CTL_MASK(x) ENABLE_LEGACY_CTL(x)
#define MULTI_IR_CTL_MASK(x) ((x)&0xff)

/*frame status*/
enum {
	STATUS_INVALI = 0,
	STATUS_NORMAL,
	STATUS_REPEAT,
};

enum {
	MULTI_CTL = 0,
	LEGACY_CTL,
	ERROR_CTL,
};

/*register file*/
enum xIRReg {
	REG_LDR_ACTIVE = 0x00 << 2,
	REG_LDR_IDLE = 0x01 << 2,
	REG_LDR_REPEAT = 0x02 << 2,
	REG_BIT_0 = 0x03 << 2,
	REG_REG0 = 0x04 << 2,
	REG_FRAME = 0x05 << 2,
	REG_STATUS = 0x06 << 2,
	REG_REG1 = 0x07 << 2,
	REG_REG2 = 0x08 << 2,
	REG_DURATN2 = 0x09 << 2,
	REG_DURATN3 = 0x0a << 2,
	REG_FRAME1 = 0x0b << 2,
	REG_STATUS1 = 0x0c << 2,
	REG_STATUS2 = 0x0d << 2,
	REG_REG3 = 0x0e << 2,
	REG_FRAME_RSV0 = 0x0f << 2,
	REG_FRAME_RSV1 = 0x10 << 2,
	REG_IRQ_CTL = 0x12 << 2,
	REG_FIFO = 0x13 << 2,
	REG_WITH = 0x14 << 2,
	REG_REPEAT_DET = 0x15 << 2,
	REG_DEMOD_CNTL0 = 0x20 << 2,
	REG_DEMOD_CNTL1 = 0x21 << 2,
	REG_DEMOD_IIR_THD = 0x22 << 2,
	REG_DEMOD_THD0 = 0x23 << 2,
	REG_DEMOD_THD1 = 0x24 << 2,
	REG_DEMOD_SUM_CNT0 = 0x25 << 2,
	REG_DEMOD_SUM_CNT1 = 0x26 << 2,
	REG_DEMOD_CNT0 = 0x27 << 2,
	REG_DEMOD_CNT1 = 0x28 << 2,
};

struct xRegList {
	uint8_t ucReg;
	uint32_t ulVal;
};

/* protocol-->register */
struct xRegProtocolMethod {
	uint8_t ucProtocol;
	uint8_t ucRegNum;
	struct xRegList *RegList;
};

/*IR Driver data*/
struct xIRDrvData {
	uint8_t ucWorkCTL : 1;
	uint8_t ucIsInit : 1;
	uint8_t ucIRStatus : 4;
	uint8_t ucPowerKeyNum;
	uint16_t ucCurWorkMode;
	uint32_t ulFrameCode;
	struct IRPowerKey *ulPowerKeyList;
	void (*vIRHandler)(struct IRPowerKey *pkey);
};

const struct xRegProtocolMethod **pGetSupportProtocol(void);
struct xIRDrvData *pGetIRDrvData(void);

#endif
