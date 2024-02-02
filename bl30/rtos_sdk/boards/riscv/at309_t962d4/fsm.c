/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h" /* Must come first. */
#include "task.h" /* RTOS task related API prototypes. */
#include "queue.h" /* RTOS queue related API prototypes. */
#include "timers.h" /* Software timer related API prototypes. */
#include "semphr.h" /* Semaphore related API prototypes. */

#include <stdio.h>
#include <unistd.h>

#include "n200_func.h"
#include "common.h"
#include "riscv_encoding.h"
#include "mailbox-api.h"
#include "irq.h"


enum PM_E {
	PM_CPU_PWR,
	PM_CPU_CORE0,
	PM_CPU_CORE1,
	PM_CPU_CORE2,
	PM_CPU_CORE3,
};

static void *xMboxCoreFsmIdle(void *msg)
{
	enum PM_E domain = *(uint32_t *)msg;

	switch (domain) {
	case PM_CPU_CORE0:
		REG32_UPDATE_BITS(ISA_SOFT_IRQ, (1 << 0), 0);
		EnableIrq(IRQ_NUM_OUT_0);
		break;
	case PM_CPU_CORE1:
		REG32_UPDATE_BITS(ISA_SOFT_IRQ, (1 << 1), 0);
		EnableIrq(IRQ_NUM_OUT_1);
		break;
	case PM_CPU_CORE2:
		REG32_UPDATE_BITS(ISA_SOFT_IRQ, (1 << 2), 0);
		EnableIrq(IRQ_NUM_OUT_2);
		break;
	case PM_CPU_CORE3:
		REG32_UPDATE_BITS(ISA_SOFT_IRQ, (1 << 3), 0);
		EnableIrq(IRQ_NUM_OUT_3);
		break;
	default:
		break;
	}
	return NULL;
}

static void xSetCoreFsmAwakeIrq(int cpuid)
{
	REG32_UPDATE_BITS(ISA_SOFT_IRQ, (1 << cpuid), (1 << cpuid));
}
static void xCore0FsmIdleHandleIsr(void)
{
	xSetCoreFsmAwakeIrq(0);
	DisableIrq(IRQ_NUM_OUT_0);
}

static void xCore1FsmIdleHandleIsr(void)
{
	xSetCoreFsmAwakeIrq(1);
	DisableIrq(IRQ_NUM_OUT_1);
}

static void xCore2FsmIdleHandleIsr(void)
{
	xSetCoreFsmAwakeIrq(2);
	DisableIrq(IRQ_NUM_OUT_2);
}

static void xCore3FsmIdleHandleIsr(void)
{
	xSetCoreFsmAwakeIrq(3);
	DisableIrq(IRQ_NUM_OUT_3);
}

void vCoreFsmIdleInit(void)
{
	int ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AOTEE_CHANNEL, MBX_CMD_CPU_FSM_IDLE,
						    xMboxCoreFsmIdle, 0);
	if (ret)
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_CPU_FSM_IDLE);

	RegisterIrq(IRQ_NUM_OUT_0, 1, xCore0FsmIdleHandleIsr);
	RegisterIrq(IRQ_NUM_OUT_1, 1, xCore1FsmIdleHandleIsr);
	RegisterIrq(IRQ_NUM_OUT_2, 1, xCore2FsmIdleHandleIsr);
	RegisterIrq(IRQ_NUM_OUT_3, 1, xCore3FsmIdleHandleIsr);
	REG32_UPDATE_BITS(SEC_SYS_CPU_CFG10, 0xF << 10, 0xF << 10);
}
