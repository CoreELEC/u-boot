/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdint.h>
#include "aml_stackprotector.h"
#include "gcc_compiler_attributes.h"
#ifdef CONFIG_BACKTRACE
#include "stack_trace.h"
#endif
#ifdef CONFIG_RISCV
#include "riscv_encoding.h"
#endif

uintptr_t __stack_chk_guard = 0x5A5A5A5A;

void __weak additional_message_hook(void *address)
{
	printf("stack smashing detected:0x%x\n", address);
}

void __stack_chk_fail(void)
{
#ifdef CONFIG_ARM64
	void *ra;

	/* Disable global interrupt */
	asm volatile ("MSR DAIFSET, #2" ::: "memory");
	asm volatile ("DSB SY");
	asm volatile ("ISB SY");

	asm volatile("mov %0, x30" : "=r" (ra));
	additional_message_hook((void *)(ra - 4));
#elif defined(CONFIG_RISCV)
	uint32_t stack_chk_func;

	/* Disable global interrupt */
	clear_csr(mstatus, MSTATUS_MIE);

	/* Do not add functions before this part for locating ra */
	asm volatile ("mv %0, ra" : "=r"(stack_chk_func));
	additional_message_hook((void *)(stack_chk_func - 4));
#else
	printf("stack smashing detected\n");
#endif

#ifdef CONFIG_BACKTRACE
	dump_stack();
#endif
	while (1)
		;
}
