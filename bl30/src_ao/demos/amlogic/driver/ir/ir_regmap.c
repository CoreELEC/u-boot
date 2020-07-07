/*
 * Copyright (C)2018 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FreeRTOS.h"
#include <common.h>
#include "ir_drv.h"

static struct xRegList xNECLegacyRegList[] = {
        {REG_LDR_ACTIVE,    (500 << 16) | (400 << 0)},
        {REG_LDR_IDLE,      300 << 16 | 200 << 0},
        {REG_LDR_REPEAT,    150 << 16 | 80 << 0},
        {REG_BIT_0,         72 << 16 | 40 << 0 },
        {REG_REG0,          7 << 28 | (0xFA0 << 12) | 0x13},
        {REG_STATUS,        (134 << 20) | (90 << 10)},
        {REG_REG1,          0xbe00},
};

static struct xRegList xNECRegList[] = {
	{REG_LDR_ACTIVE, (500 << 16) | (400 << 0)},
	{REG_LDR_IDLE, 300 << 16 | 200 << 0},
	{REG_LDR_REPEAT, 150 << 16 | 80 << 0},
	{REG_BIT_0, 72 << 16 | 40 << 0},
	{REG_REG0, 7 << 28 | (0xFA0 << 12) | 0x13},
	{REG_STATUS, (134 << 20) | (90 << 10)},
	{REG_REG1, 0x9f00},
	{REG_REG2, 0x00},
	{REG_DURATN2, 0x00},
	{REG_DURATN3, 0x00}
};

static xRegProtocolMethod xNECLegacyDecode = {
	.ucProtocol = MODE_HARD_LEAGCY_NEC,
	.RegList = xNECLegacyRegList,
	.ucRegNum = ARRAY_SIZE(xNECLegacyRegList),
};

static xRegProtocolMethod xNECDecode = {
	.ucProtocol = MODE_HARD_NEC,
	.RegList = xNECRegList,
	.ucRegNum = ARRAY_SIZE(xNECRegList),
};

static const xRegProtocolMethod *xSupportProtocol[] = {
	&xNECDecode,
	&xNECLegacyDecode,
	NULL,
};

static struct xIRDrvData xIRDrvPrvData;

struct xIRDrvData *pGetIRDrvData(void)
{
	return &xIRDrvPrvData;
}

const xRegProtocolMethod **pGetSupportProtocol(void)
{
	return xSupportProtocol;
}
