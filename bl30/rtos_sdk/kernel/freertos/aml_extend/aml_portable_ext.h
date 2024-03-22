/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_PORTABLE_EXT_H__
#define __AML_PORTABLE_EXT_H__

#include <stdint.h>

typedef enum Halt_Action{
	HLTACT_RUN_OS=0,
	HLTACT_SHUTDOWN_SYSTEM,
	HLTACT_EXCEPTION_SYSTEM
}Halt_Action_e;

#if CONFIG_FTRACE
extern void vTraceDisInterrupt(void);
extern void vTraceEnInterrupt(void);
#endif

#ifdef CONFIG_ARM64
static inline unsigned long _irq_save(void)
{
	unsigned long flags;

	asm volatile(
		"mrs	%0, daif		// arch_local_irq_save\n"
		"msr	daifset, #2"
		: "=r" (flags)
		:
		: "memory");
#if CONFIG_FTRACE
	vTraceDisInterrupt();
#endif
	return flags;
}
static inline void _irq_restore(unsigned long flags)
{
	asm volatile(
		"msr	daif, %0		// arch_local_irq_restore"
	:
	: "r" (flags)
	: "memory");
#if CONFIG_FTRACE
	if ((flags & 0x2) == 0)
		vTraceEnInterrupt();
#endif
}

#define portIRQ_SAVE(flags) 			\
	do {								\
		flags = _irq_save();			\
	} while (0)

#define portIRQ_RESTORE(flags)			\
	do {								\
		_irq_restore(flags);			\
	} while (0)
#else

#define portIRQ_SAVE(a)		(void)(a)
#define portIRQ_RESTORE(a)	(void)(a)

#define portIRQ_SAVE(a)		(void)(a)
#define portIRQ_RESTORE(a)	(void)(a)

#endif

typedef void (*ipi_process_handle)(void *);

void vLowPowerSystem(void);

unsigned int xPortIsIsrContext(void);

void vPortAddIrq(uint32_t irq_num);

void vPortRemoveIrq(uint32_t irq_num);

void vPortRtosInfoUpdateStatus(uint32_t status);

void vPortHaltSystem(Halt_Action_e act);

void *pvPortRealloc(void *ptr, size_t size);

void vHardwareResourceRecord(void);

void vHardwareResourceRelease(void);

int xRtosLoadStageIndicator(void);

extern void _global_constructors(void);

void xIpiProcessCallbackRegister(ipi_process_handle handler);

void xIpiCommonProcess(void *args);

#endif
