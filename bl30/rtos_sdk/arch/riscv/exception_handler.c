/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "exception_handler.h"
#include "riscv_encoding.h"
#include "interrupt_control_eclic.h"
#ifdef CONFIG_DEBUG_COREDUMP
#include "coredump.h"
#endif

#if __riscv_xlen == 64
    #define portWORD_SIZE 8
#elif __riscv_xlen == 32
    #define portWORD_SIZE 4
#else
    #error Assembler did not define __riscv_xlen
#endif

static void dump_stack(uint32_t sp)
{
	// print x1
	printf("x%-2d: %08x,", 1, *(unsigned int *)(sp + 1 * portWORD_SIZE));

	// print x5-x31
	for (int i = 5; i < 32; i++) {
		if (i % 2)
			printf("x%-2d: %08x\n", i,
				*(unsigned int *)(sp + ((i - 3) * portWORD_SIZE)));
		else
			printf("x%-2d: %08x,", i,
				*(unsigned int *)(sp + ((i - 3) * portWORD_SIZE)));
	}
}

static uint32_t handle_exception(uint32_t mcause, uint32_t sp)
{
	uint8_t exception_type;
	uint32_t mstatus_mps_bits;

	clear_csr(mstatus, MSTATUS_MIE);

	exception_type = (mcause & 0x1f);
	mstatus_mps_bits = ((read_csr(mstatus) & MSTATUS_MPS) >> MSTATUS_MPS_LSB);
	// 0 means prior status is normal,2 EXCEPTION,3 NMI,1 int
	printf("exception mstatus.MPS %lx\n", mstatus_mps_bits);
	// exception position
	printf("exception mepc %lx\n", read_csr(mepc));
	// exception instruction
	printf("exception mtval %lx\n", read_csr(mbadaddr));
	printf("exception mcause %lx\n", mcause);

	switch (exception_type) {
	case 0:
		printf("Instruction address misaligned!\n");
		break;

	case 1:
		printf("Instruction access fault!\n");
		break;

	case 2:
		printf("Illegal instruction\n");
		break;

	case 3:
		printf("Breakpoint\n");
		break;

	case 4:
		printf("Load address misaligned\n");
		break;

	case 5:
		printf("Load access fault\n");
		break;

	case 6:
		printf("Store/AMO address misaligned\n");
		break;

	case 7:
		printf("Store/AMO access fault\n");
		break;

	case 11:
		printf("Environment call from M-mode\n");
		break;

	default:
		printf("unknown exception: %lx ", exception_type);
		break;
	}

	dump_stack(sp);

#ifdef CONFIG_DEBUG_COREDUMP
	coredump(0, (void *)sp);
#endif

	do {
	} while (1);

	return 0;
}

static uint32_t handle_nmi(uint32_t mcause, uint32_t sp)
{
	printf("nmi mepc ", read_csr(mepc));
	printf("nmi mtval ", read_csr(mbadaddr));
	printf("nmi mcause ", mcause);
	return 0;
}

uint32_t interrupt_register_exception(uint32_t mcause, uint32_t sp)
{
	return handle_exception(mcause, sp);
}

uint32_t interrupt_register_nmi(uint32_t mcause, uint32_t sp)
{
	return handle_nmi(mcause, sp);
}
