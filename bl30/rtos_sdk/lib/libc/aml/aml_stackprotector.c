/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdint.h>
#include "aml_stackprotector.h"
#ifdef CONFIG_BACKTRACE
#include "stack_trace.h"
#endif
#ifdef CONFIG_RISCV
#include "riscv_encoding.h"
#endif

uintptr_t __stack_chk_guard = 0x5A5A5A5A;

void __stack_chk_fail(void)
{
#ifdef CONFIG_RISCV
	/* Disable global interrupt */
	clear_csr(mstatus, MSTATUS_MIE);

	uint32_t stack_chk_func;
	/* Do not add functions before this part for locating ra */
	asm volatile ("mv %0, ra" : "=r"(stack_chk_func));
#endif
	printf("stack smashing detected ...");
#ifdef CONFIG_RISCV
	printf("in bl30, stop here!\n");
	printf("The last addr of smashing function: 0x%x\n", (stack_chk_func - 4));
#endif
#ifdef CONFIG_BACKTRACE
	dump_stack();
#endif
	while (1)
		;
}
