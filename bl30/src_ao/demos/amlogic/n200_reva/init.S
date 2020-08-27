/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Cortex-M CPU initialization
 */

//#include "config.h"

.section .text.vecttable
.globl _start
.type _start,@function

#define MSTATUS_MIE         0x00000008

_start:
	csrc mstatus, MSTATUS_MIE  //Disable the interrupt globally
.option push
.option norelax
	//la gp, __global_pointer$
.option pop
	la sp, stack_end
    #csrrsi x0, 0xbfe, 0x8 // Disable I-Cache

#if 0
	/* Load data section */
	la a0, _data_lma
	la a1, _data
	la a2, _edata
	bgeu a1, a2, 2f
1:
	lw t0, (a0)
	sw t0, (a1)
	addi a0, a0, 4
	addi a1, a1, 4
	bltu a1, a2, 1b
2:
#endif

	/* Clear bss section */
	la a0, __bss_start
	la a1, __bss_end
	bgeu a0, a1, 2f
1:
	sw zero, (a0)
	addi a0, a0, 4
	bltu a0, a1, 1b
2:


	/* argc = argv = 0 */
	li a0, 0
	li a1, 0
	call main

1:
	j 1b


/* Reserve space for system stack */
.section .bss.system_stack
stack_start:
.space CONFIG_STACK_SIZE, 0
stack_end:
.global stack_end

