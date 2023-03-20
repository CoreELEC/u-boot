/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include <common.h>
#include "ir.h"
#include "ir_drv.h"

static struct xRegList xNECLegacyRegList[] = {
	{ REG_LDR_ACTIVE, (500 << 16) | (400 << 0) },
	{ REG_LDR_IDLE, 300 << 16 | 200 << 0 },
	{ REG_LDR_REPEAT, 150 << 16 | 80 << 0 },
	{ REG_BIT_0, 72 << 16 | 40 << 0 },
	{ REG_REG0, 7 << 28 | (0xFA0 << 12) | 0x13 },
	{ REG_STATUS, (134 << 20) | (90 << 10) },
	{ REG_REG1, 0xbe00 },
};

static struct xRegList xNEC32KRegList[] = { { REG_LDR_ACTIVE, (327 << 16) | (262 << 0) },
					 { REG_LDR_IDLE, 196 << 16 | 131 << 0 },
					 { REG_LDR_REPEAT, 98 << 16 | 52 << 0 },
					 { REG_BIT_0, 47 << 16 | 26 << 0 },
					 { REG_REG0, 1 << 28 | (0xA3C << 12) | 0x0 },
					 { REG_STATUS, (85 << 20) | (57 << 10) },
					 { REG_REG1, 0x9f00 | (1 << 14) | (1 << 29) | (1 << 30) },
					 { REG_REG2, 0x00 },
					 { REG_DURATN2, 0x00 },
					 { REG_DURATN3, 0x00 } };

static struct xRegList xNECRegList[] = { { REG_LDR_ACTIVE, (500 << 16) | (400 << 0) },
					 { REG_LDR_IDLE, 300 << 16 | 200 << 0 },
					 { REG_LDR_REPEAT, 150 << 16 | 80 << 0 },
					 { REG_BIT_0, 72 << 16 | 40 << 0 },
					 { REG_REG0, 7 << 28 | (0xFA0 << 12) | 0x13 },
					 { REG_STATUS, (134 << 20) | (90 << 10) },
					 { REG_REG1, 0x9f00 },
					 { REG_REG2, 0x00 },
					 { REG_DURATN2, 0x00 },
					 { REG_DURATN3, 0x00 } };

#ifdef MODE_HARD_DUOKAN
static struct xRegList xDUOKANRegList[] = { { REG_LDR_ACTIVE, ((70 << 16) | (30 << 0)) },
					    { REG_LDR_IDLE, ((50 << 16) | (15 << 0)) },
					    { REG_LDR_REPEAT, ((30 << 16) | (26 << 0)) },
					    { REG_BIT_0, ((66 << 16) | (40 << 0)) },
					    { REG_REG0, ((3 << 28) | (0x4e2 << 12) | (0x13)) },
					    { REG_STATUS, ((80 << 20) | (66 << 10)) },
					    { REG_REG1, 0x9300 },
					    { REG_REG2, 0xb90b },
					    { REG_DURATN2, ((97 << 16) | (80 << 0)) },
					    { REG_DURATN3, ((120 << 16) | (97 << 0)) },
					    { REG_REG3, 5000 << 0 } };

static struct xRegProtocolMethod xDUOKANDecode = {
	.ucProtocol = MODE_HARD_DUOKAN,
	.RegList = xDUOKANRegList,
	.ucRegNum = ARRAY_SIZE(xDUOKANRegList),
};
#endif

#ifdef MODE_HARD_XMP_1
static struct xRegList xXMP1RegList[] = { { REG_LDR_ACTIVE, 0 },
					  { REG_LDR_IDLE, 0 },
					  { REG_LDR_REPEAT, 0 },
					  { REG_BIT_0, (52 << 16) | (45 << 0) },
					  { REG_REG0, ((7 << 28) | (0x5DC << 12) | (0x13)) },
					  { REG_STATUS, (87 << 20) | (80 << 10) },
					  { REG_REG1, 0x9f00 },
					  { REG_REG2, 0xa90e },
					  /*n=10,758+137*10=2128us,2128/20= 106*/
					  { REG_DURATN2, (121 << 16) | (114 << 0) },
					  { REG_DURATN3, (7 << 16) | (7 << 0) },
					  { REG_REG3, 0 } };

static struct xRegProtocolMethod xXMP1Decode = {
	.ucProtocol = MODE_HARD_XMP_1,
	.RegList = xXMP1RegList,
	.ucRegNum = ARRAY_SIZE(xXMP1RegList),
};
#endif

#ifdef MODE_HARD_RC5
static struct xRegList xRC5RegList[] = {
	{ REG_LDR_ACTIVE, 0 },
	{ REG_LDR_IDLE, 0 },
	{ REG_LDR_REPEAT, 0 },
	{ REG_BIT_0, 0 },
	{ REG_REG0, ((3 << 28) | (0x1644 << 12) | 0x13) },
	{ REG_STATUS, (1 << 30) },
	{ REG_REG1, ((1 << 15) | (13 << 8)) },
	/*bit[0-3]: RC5; bit[8]: MSB first mode; bit[11]: compare frame method*/
	{ REG_REG2, ((1 << 13) | (1 << 11) | (1 << 8) | 0x7) },
	/*Half bit for RC5 format: 888.89us*/
	{ REG_DURATN2, ((53 << 16) | (38 << 0)) },
	/*RC5 typically 1777.78us for whole bit*/
	{ REG_DURATN3, ((99 << 16) | (81 << 0)) },
	{ REG_REG3, 0 }
};

static struct xRegProtocolMethod xRC5Decode = {
	.ucProtocol = MODE_HARD_RC5,
	.RegList = xRC5RegList,
	.ucRegNum = ARRAY_SIZE(xRC5RegList),
};
#endif

#ifdef MODE_HARD_RC6
static struct xRegList xRC6RegList[] = {
	{ REG_LDR_ACTIVE, (210 << 16) | (125 << 0) },
	{ REG_LDR_IDLE, 50 << 16 | 38 << 0 }, /* leader idle 400*/
	{ REG_LDR_REPEAT, 145 << 16 | 125 << 0 }, /* leader repeat*/
	/* logic '0' or '00' 1500us*/
	{ REG_BIT_0, 51 << 16 | 38 << 0 },
	{ REG_REG0, (7 << 28) | (0xFA0 << 12) | 0x13 },
	/* sys clock boby time.base time = 20 body frame*/
	{ REG_STATUS, (94 << 20) | (82 << 10) },
	/*20bit:9440 32bit:9f40 36bit:a340 37bit:a440*/
	{ REG_REG1, 0xa440 },
	/*it may get the wrong customer value and key value from register if
	 *the value is set to 0x4,so the register value must set to 0x104
	 */
	{ REG_REG2, 0x2909 },
	{ REG_DURATN2, ((28 << 16) | (16 << 0)) },
	{ REG_DURATN3, ((51 << 16) | (38 << 0)) },
};

static struct xRegProtocolMethod xRC6Decode = {
	.ucProtocol = MODE_HARD_RC6,
	.RegList = xRC6RegList,
	.ucRegNum = ARRAY_SIZE(xRC6RegList),
};
#endif

#ifdef MODE_HARD_TOSHIBA
static struct xRegList xTOSHIBARegList[] = {
	{ REG_LDR_ACTIVE, (280 << 16) | (180 << 0) },
	{ REG_LDR_IDLE, (280 << 16) | (180 << 0) },
	{ REG_LDR_REPEAT, (150 << 16) | (60 << 0) },
	{ REG_BIT_0, (72 << 16) | (40 << 0) },
	{ REG_REG0, (7 << 28) | (0xFA0 << 12) | 0x13 },
	{ REG_STATUS, (134 << 20) | (90 << 10) },
	{ REG_REG1, 0x9f00 },
	{ REG_REG2, (0x05) | (1 << 24) | (23 << 11) },
	{ REG_DURATN2, 0x00 },
	{ REG_DURATN3, 0x00 },
	{ REG_REPEAT_DET, (1 << 31) | (0xFA0 << 16) | (10 << 0) },
	{ REG_REG3, 0x2AF8 },
};

static struct xRegProtocolMethod xTOSHIBADecode = {
	.ucProtocol = MODE_HARD_TOSHIBA,
	.RegList = xTOSHIBARegList,
	.ucRegNum = ARRAY_SIZE(xTOSHIBARegList),
};
#endif

#ifdef MODE_HARD_RCA
static struct xRegList xRCARegList[] = { { REG_LDR_ACTIVE, (250 << 16) | (160 << 0) },
					 { REG_LDR_IDLE, 250 << 16 | 160 << 0 },
					 { REG_LDR_REPEAT, 250 << 16 | 160 << 0 },
					 { REG_BIT_0, 100 << 16 | 48 << 0 },
					 { REG_REG0, 7 << 28 | (0xFA0 << 12) | 0x13 },
					 { REG_STATUS, (150 << 20) | (110 << 10) },
					 { REG_REG1, 0x9700 },
					 { REG_REG2, 0x104 | (1 << 24) | (23 << 11) },
					 { REG_DURATN2, 0x00 },
					 { REG_REPEAT_DET, (1 << 31) | (0xFA0 << 16) | (10 << 0) },
					 { REG_REG3, 0x1A00 },
					 { REG_DURATN3, 0x00 } };

static struct xRegProtocolMethod xRCADecode = {
	.ucProtocol = MODE_HARD_RCA,
	.RegList = xRCARegList,
	.ucRegNum = ARRAY_SIZE(xRCARegList),
};

#endif

#ifdef MODE_HARD_RCMM
static struct xRegList xRCMMRegList[] = {
	{ REG_LDR_ACTIVE, (35 << 16) | (17 << 0) },
	{ REG_LDR_IDLE, (17 << 16) | (8 << 0) },
	{ REG_LDR_REPEAT, (31 << 16) | (11 << 0) },
	{ REG_BIT_0, (25 << 16) | (21 << 0) },
	{ REG_REG0, (7 << 28) | (0x590 << 12) | 0x13 },
	{ REG_STATUS, (36 << 20) | (29 << 10) },
	{ REG_REG1, 0x9f00 },
	{ REG_REG2, 0x1150a },
	{ REG_DURATN2, ((43 << 16) | (37 << 0)) },
	{ REG_DURATN3, ((50 << 16) | (44 << 0)) },
	{ REG_REG3, 1200 << 0 },
};

static struct xRegProtocolMethod xRCMMDecode = {
	.ucProtocol = MODE_HARD_RCMM,
	.RegList = xRCMMRegList,
	.ucRegNum = ARRAY_SIZE(xRCMMRegList),
};
#endif

static struct xRegProtocolMethod xNECLegacyDecode = {
	.ucProtocol = MODE_HARD_LEAGCY_NEC,
	.RegList = xNECLegacyRegList,
	.ucRegNum = ARRAY_SIZE(xNECLegacyRegList),
};

static struct xRegProtocolMethod xNECDecode = {
	.ucProtocol = MODE_HARD_NEC,
	.RegList = xNECRegList,
	.ucRegNum = ARRAY_SIZE(xNECRegList),
};

static struct xRegProtocolMethod xNEC32KDecode = {
	.ucProtocol = MODE_HARD_NEC_32K,
	.RegList = xNEC32KRegList,
	.ucRegNum = ARRAY_SIZE(xNEC32KRegList),
};

static const struct xRegProtocolMethod *xSupportProtocol[] = {
	&xNECDecode,
	&xNEC32KDecode,
	&xNECLegacyDecode,

#ifdef MODE_HARD_DUOKAN
	&xDUOKANDecode,
#endif

#ifdef MODE_HARD_RCA
	&xRCADecode,
#endif

#ifdef MODE_HARD_RC5
	&xRC5Decode,
#endif

#ifdef MODE_HARD_RC6
	&xRC6Decode,
#endif

#ifdef MODE_HARD_TOSHIBA
	&xTOSHIBADecode,
#endif

#ifdef MODE_HARD_XMP_1
	&xXMP1Decode,
#endif

#ifdef MODE_HARD_RCMM
	&xRCMMDecode,
#endif
	NULL,
};

static struct xIRDrvData xIRDrvPrvData;

struct xIRDrvData *pGetIRDrvData(void)
{
	return &xIRDrvPrvData;
}

const struct xRegProtocolMethod **pGetSupportProtocol(void)
{
	return xSupportProtocol;
}
