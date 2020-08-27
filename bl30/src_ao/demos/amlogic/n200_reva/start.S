// See LICENSE for license details.

// See LICENSE for license details.

#include "riscv_encoding.h"

	.section .init
	.globl _start
	.type _start,@function


_start:
	csrc mstatus, MSTATUS_MIE  //Disable the interrupt globally
.option push
.option norelax
	//la gp, __global_pointer$
.option pop
	la sp, _sp
    #csrrsi x0, 0xbfe, 0x8 // Disable I-Cache

	/* Clear bss section */
	la a0, _bss_start
	la a1, _bss_end
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

_bss_end:
.long _ebss
_bss_start:
.long _bss



