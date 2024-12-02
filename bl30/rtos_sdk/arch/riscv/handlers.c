/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "riscv_encoding.h"
#include "gcc_compiler_attributes.h"
#include "n200_func.h"

__weak uintptr_t handle_nmi(void)
{
	printf("\nhandle_nmi");

	while (1)
		;

	return 0;
}

__weak uintptr_t handle_trap(uintptr_t mcause, uintptr_t sp)
{
	(void)sp;

	if ((mcause & 0xFFF) == 0xFFF)
		handle_nmi();
	//write(1, "trap\n", 5);
	printf("In trap handler, the mcause is %d\n", mcause);
	printf("In trap handler, the mepc is 0x%lx\n", read_csr(mepc));
	printf("In trap handler, the mtval is 0x%lx\n", read_csr(mbadaddr));
	//_exit(mcause);
	printf("\nhandle_trap");

	while (1)
		;

	return 0;
}

#ifdef CONFIG_N200_REVA
__weak uint32_t handle_irq(uint32_t int_num)
{
	// Enable interrupts to allow interrupt preempt based on priority
	//set_csr(mstatus, MSTATUS_MIE);

	pic_interrupt_handlers[int_num]();
	/* Since it will complete in the assembly instructions, it is redundant in this place. */
	//pic_complete_interrupt(int_num);
	// Disable interrupts
	//clear_csr(mstatus, MSTATUS_MIE);
	return int_num;
}
#endif
