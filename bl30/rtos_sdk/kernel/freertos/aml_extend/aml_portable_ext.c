/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_portable_ext.h"

#include "FreeRTOS.h"
#include "arm-smccc.h"
#include "rtosinfo.h"
#include "gic.h"
#include "task.h"


#define portMAX_IRQ_NUM 1024

#ifndef configPREPARE_CPU_HALT
#define configPREPARE_CPU_HALT()
#endif

extern xRtosInfo_t xRtosInfo;

static unsigned char irq_mask[portMAX_IRQ_NUM / 8];

static void pvPortSetIrqMask(uint32_t irq_num, int val)
{
	int idx, bit;
	unsigned long flags;

	portIRQ_SAVE(flags);
	bit = (irq_num & 0x7);
	idx = (irq_num / 8);
	if (val)
		irq_mask[idx] |= (1 << bit);
	else
		irq_mask[idx] &= ~(1 << bit);
	portIRQ_RESTORE(flags);
}

static unsigned long prvCorePowerDown(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(0x84000002, 0, 0, 0, 0, 0, 0, 0, &res);
	return res.a0;
}

void vLowPowerSystem(void)
{
	taskENTER_CRITICAL();
	portDISABLE_INTERRUPTS();
	vPortRtosInfoUpdateStatus(eRtosStat_Done);
	/*set mailbox to dsp for power control!*/
	while (1) {
		__asm volatile("wfi");
	}
}

uint8_t xPortIsIsrContext(void)
{
#if CONFIG_ARM64
	return ullPortInterruptNesting == 0 ? 0 : 1;
#else
	return ulPortInterruptNesting == 0 ? 0 : 1;
#endif
}

void vPortAddIrq(uint32_t irq_num)
{
	if (irq_num >= portMAX_IRQ_NUM)
		return;
	pvPortSetIrqMask(irq_num, 1);
}
void vPortRemoveIrq(uint32_t irq_num)
{
	if (irq_num >= portMAX_IRQ_NUM)
		return;
	pvPortSetIrqMask(irq_num, 0);
}

void vPortRtosInfoUpdateStatus(uint32_t status)
{
	xRtosInfo.status = status;
	vCacheFlushDcacheRange((unsigned long)&xRtosInfo, sizeof(xRtosInfo));
}

void vPortHaltSystem(Halt_Action_e act)
{
	uint32_t irq = 0, i;

	taskENTER_CRITICAL();
	portDISABLE_INTERRUPTS();
	for (irq = 0; irq < portMAX_IRQ_NUM; irq += 8) {
		for (i = 0; i < 8; i++) {
			if (irq_mask[irq / 8] & (1 << i))
				plat_gic_irq_unregister(irq + i);
		}
	}
#if 0
	//Do not support now
	if (act == HLTACT_SHUTDOWN_SYSTEM)
		xRtosInfo.flags |= RTOSINFO_FLG_SHUTDOWN;
	else
		xRtosInfo.flags |= ~((uint32_t)RTOSINFO_FLG_SHUTDOWN);
#else
	(void)act;
#endif
	vPortRtosInfoUpdateStatus(eRtosStat_Done);
	configPREPARE_CPU_HALT();
	plat_gic_raise_softirq(1, 7);
	while (1) {
#ifdef CONFIG_SOC_T7
		prvCorePowerDown();
#else
		__asm volatile("wfi");
#endif
	}
}