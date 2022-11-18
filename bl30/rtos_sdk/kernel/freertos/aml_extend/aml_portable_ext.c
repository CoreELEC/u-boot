/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <FreeRTOS.h>
#include "gic.h"
#include "task.h"
#include "common.h"
#include "rtosinfo.h"
#include "arm-smccc.h"
#include "aml_portable_ext.h"

#define portMAX_IRQ_NUM 1024

#ifndef configPREPARE_CPU_HALT
#define configPREPARE_CPU_HALT()
#endif

extern xRtosInfo_t xRtosInfo;

static unsigned char irq_mask[portMAX_IRQ_NUM / 8];
/*-----------------------------------------------------------*/
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

/*-----------------------------------------------------------*/
static unsigned long prvCorePowerDown(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(0x84000002, 0, 0, 0, 0, 0, 0, 0, &res);
	return res.a0;
}
/*-----------------------------------------------------------*/
static void vOperationBeforeShutdown(void)
{
#if defined(CONFIG_SOC_T7) || defined(CONFIG_SOC_T7C)
	/* viu1_line_n_int */
	plat_gic_irq_register_with_default(227, 0, 0);
	/* ge2d_int */
	plat_gic_irq_register_with_default(249, 0, 1);
	/* dwap_irq */
	plat_gic_irq_register_with_default(91, 0, 1);
	/* isp adapter frontend2 irq */
	plat_gic_irq_register_with_default(343, 0, 1);
	plat_gic_irq_register_with_default(321, 1, 0);
	/* timerA irq*/
	plat_gic_irq_register_with_default(32, 0, 0);
#endif
}

/*-----------------------------------------------------------*/
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

/*-----------------------------------------------------------*/
unsigned int xPortIsIsrContext(void)
{
#if CONFIG_ARM64
	return ullPortInterruptNesting == 0 ? 0 : 1;
#else
	return ulPortInterruptNesting == 0 ? 0 : 1;
#endif
}

/*-----------------------------------------------------------*/
void vPortAddIrq(uint32_t irq_num)
{
	if (irq_num >= portMAX_IRQ_NUM)
		return;
	pvPortSetIrqMask(irq_num, 1);
}

/*-----------------------------------------------------------*/
void vPortRemoveIrq(uint32_t irq_num)
{
	if (irq_num >= portMAX_IRQ_NUM)
		return;
	pvPortSetIrqMask(irq_num, 0);
}

/*-----------------------------------------------------------*/
void vPortRtosInfoUpdateStatus(uint32_t status)
{
	xRtosInfo.status = status;
	vCacheFlushDcacheRange((unsigned long)&xRtosInfo, sizeof(xRtosInfo));
}

/*-----------------------------------------------------------*/
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

	(void)act;

	vPortRtosInfoUpdateStatus(eRtosStat_Done);

	configPREPARE_CPU_HALT();

	vHardwareResourceRelease();

	plat_gic_raise_softirq(1, 7);

	vOperationBeforeShutdown();

	while (1) {
#if defined(CONFIG_SOC_T7) || defined(CONFIG_SOC_T7C)
		prvCorePowerDown();
#else
		__asm volatile("wfi");
#endif
	}
}

/*-----------------------------------------------------------*/
#if CONFIG_BACKTRACE
#include "stack_trace.h"
int vPortTaskPtregs(TaskHandle_t task, struct pt_regs *reg)
{
	StackType_t *pxTopOfStack;
	int i;

	if (!task || task == xTaskGetCurrentTaskHandle())
		return -1;
	pxTopOfStack = *(StackType_t **)task;
	reg->sp = (unsigned long)pxTopOfStack;
	if (*pxTopOfStack)
		pxTopOfStack += 64;
	pxTopOfStack += 2;
	reg->elr = *pxTopOfStack++;
	reg->spsr = *pxTopOfStack++;
	for (i = 0; i < 31; i++)
		reg->regs[i] = pxTopOfStack[31 - (i ^ 1)];
	return 0;
}
#endif

/*-----------------------------------------------------------*/
#if CONFIG_LOG_BUFFER
void vPortConfigLogBuf(uint32_t pa, uint32_t len)
{
	xRtosInfo.logbuf_phy = pa;
	xRtosInfo.logbuf_len = len;
	vCacheFlushDcacheRange((unsigned long)&xRtosInfo, sizeof(xRtosInfo));
}
#endif

/*-----------------------------------------------------------*/
void vHardwareResourceRecord(void)
{
#if defined(CONFIG_SOC_T7) || defined(CONFIG_SOC_T7C)
extern void vTickTimerRecord(void);
	/* systick timer record */
	vTickTimerRecord();
#endif
}

/*-----------------------------------------------------------*/
void vHardwareResourceRelease(void)
{
#if defined(CONFIG_SOC_T7) || defined(CONFIG_SOC_T7C)
extern void vTickTimerRestore(void);
	/* systick timer restore */
	vTickTimerRestore();
#endif
}

/*-----------------------------------------------------------*/
int xRtosLoadStageIndicator(void)
{
	static int load_finished;

#if defined(CONFIG_BOARD_AW419_C308L)
	vCacheFlushDcacheRange((CONFIG_SCATTER_LOAD_ADDRESS - 0x04), 4);
	if (REG32(CONFIG_SCATTER_LOAD_ADDRESS - 0x04) == 0xaabbccdd) {
		if (0 == load_finished)
			_global_constructors();
		load_finished = 1;
		return 2;
	}
#endif
	return 1;
}
