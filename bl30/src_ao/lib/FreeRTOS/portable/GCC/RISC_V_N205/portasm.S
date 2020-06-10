/*
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#include "soc.h"
#include "riscv_encoding.h"
#include "n200_timer.h"
#include "n200_eclic.h"

//#include "portmacro.h"

# define STORE    sw
# define LOAD     lw
# define REGBYTES 4


#define INT_MASK 0x7FFFFFFF

.section      .text.entry
.align 4

.global portSAVE_CONTEXT
.global portRESTORE_CONTEXT
.global xPortStartScheduler
.global vPortYield
.global vPortEndScheduler
.global DISABLE_MIE

/* Macro for saving task context */
.macro pushREGFILE
	/* make room in stack */
	addi	sp, sp, -REGBYTES * 36

	/* Save Context */
	STORE	x1, 1 * REGBYTES(sp)
	STORE	x2, 2 * REGBYTES(sp)
	//STORE	x3, 3 * REGBYTES(sp)
	STORE	x4, 4 * REGBYTES(sp)
	STORE	x5, 5 * REGBYTES(sp)
	STORE	x6, 6 * REGBYTES(sp)
	STORE	x7, 7 * REGBYTES(sp)
	STORE	x8, 8 * REGBYTES(sp)
	STORE	x9, 9 * REGBYTES(sp)
	STORE	x10, 10 * REGBYTES(sp)
	STORE	x11, 11 * REGBYTES(sp)
	STORE	x12, 12 * REGBYTES(sp)
	STORE	x13, 13 * REGBYTES(sp)
	STORE	x14, 14 * REGBYTES(sp)
	STORE	x15, 15 * REGBYTES(sp)
#ifndef __riscv_32e
	STORE	x16, 16 * REGBYTES(sp)
	STORE	x17, 17 * REGBYTES(sp)
	STORE	x18, 18 * REGBYTES(sp)
	STORE	x19, 19 * REGBYTES(sp)
	STORE	x20, 20 * REGBYTES(sp)
	STORE	x21, 21 * REGBYTES(sp)
	STORE	x22, 22 * REGBYTES(sp)
	STORE	x23, 23 * REGBYTES(sp)
	STORE	x24, 24 * REGBYTES(sp)
	STORE	x25, 25 * REGBYTES(sp)
	STORE	x26, 26 * REGBYTES(sp)
	STORE	x27, 27 * REGBYTES(sp)
	STORE	x28, 28 * REGBYTES(sp)
	STORE	x29, 29 * REGBYTES(sp)
	STORE	x30, 30 * REGBYTES(sp)
	STORE	x31, 31 * REGBYTES(sp)
#endif
.endm

.macro pushREGFILE_Caller
	/* make room in stack */
	addi	sp, sp, -REGBYTES * 36

	/* Save Context */
	STORE	x1, 1 * REGBYTES(sp)
	//STORE	x2, 2 * REGBYTES(sp)
	////STORE	x3, 3 * REGBYTES(sp)
	//STORE	x4, 4 * REGBYTES(sp)
	STORE	x5, 5 * REGBYTES(sp)
	STORE	x6, 6 * REGBYTES(sp)
	STORE	x7, 7 * REGBYTES(sp)
	//STORE	x8, 8 * REGBYTES(sp)
	//STORE	x9, 9 * REGBYTES(sp)
	STORE	x10, 10 * REGBYTES(sp)
	STORE	x11, 11 * REGBYTES(sp)
	STORE	x12, 12 * REGBYTES(sp)
	STORE	x13, 13 * REGBYTES(sp)
	STORE	x14, 14 * REGBYTES(sp)
	STORE	x15, 15 * REGBYTES(sp)
#ifndef  __riscv_32e
	STORE	x16, 16 * REGBYTES(sp)
	STORE	x17, 17 * REGBYTES(sp)
	//STORE	x18, 18 * REGBYTES(sp)
	//STORE	x19, 19 * REGBYTES(sp)
	//STORE	x20, 20 * REGBYTES(sp)
	//STORE	x21, 21 * REGBYTES(sp)
	//STORE	x22, 22 * REGBYTES(sp)
	//STORE	x23, 23 * REGBYTES(sp)
	//STORE	x24, 24 * REGBYTES(sp)
	//STORE	x25, 25 * REGBYTES(sp)
	//STORE	x26, 26 * REGBYTES(sp)
	//STORE	x27, 27 * REGBYTES(sp)
	STORE	x28, 28 * REGBYTES(sp)
	STORE	x29, 29 * REGBYTES(sp)
	STORE	x30, 30 * REGBYTES(sp)
	STORE	x31, 31 * REGBYTES(sp)
	#endif
.endm

/* Macro for saving task context */
.macro pushREGFILE_Callee

	/* Save Context */
	//STORE	x1, 1 * REGBYTES(sp)
	STORE	x2, 2 * REGBYTES(sp)
	////STORE	x3, 3 * REGBYTES(sp)
	STORE	x4, 4 * REGBYTES(sp)
//	STORE	x5, 5 * REGBYTES(sp)
	//STORE	x6, 6 * REGBYTES(sp)
	//STORE	x7, 7 * REGBYTES(sp)
	STORE	x8, 8 * REGBYTES(sp)
	STORE	x9, 9 * REGBYTES(sp)
	//STORE	x10, 10 * REGBYTES(sp)
	//STORE	x11, 11 * REGBYTES(sp)
	//STORE	x12, 12 * REGBYTES(sp)
	//STORE	x13, 13 * REGBYTES(sp)
	//STORE	x14, 14 * REGBYTES(sp)
	//STORE	x15, 15 * REGBYTES(sp)
	//STORE	x16, 16 * REGBYTES(sp)
	//STORE	x17, 17 * REGBYTES(sp)
#ifndef __riscv_32e
	STORE	x18, 18 * REGBYTES(sp)
	STORE	x19, 19 * REGBYTES(sp)
	STORE	x20, 20 * REGBYTES(sp)
	STORE	x21, 21 * REGBYTES(sp)
	STORE	x22, 22 * REGBYTES(sp)
	STORE	x23, 23 * REGBYTES(sp)
	STORE	x24, 24 * REGBYTES(sp)
	STORE	x25, 25 * REGBYTES(sp)
	STORE	x26, 26 * REGBYTES(sp)
	STORE	x27, 27 * REGBYTES(sp)
	//STORE	x28, 28 * REGBYTES(sp)
	//STORE	x29, 29 * REGBYTES(sp)
	//STORE	x30, 30 * REGBYTES(sp)
	//STORE	x31, 31 * REGBYTES(sp)
#endif
.endm


/*saves mstatus and tcb	*/
.macro portSAVE_CONTEXT_EXCP
	.global	pxCurrentTCB
	/* Store mstatus */
	csrr	t0, CSR_MSTATUS	//pointer
	STORE	t0, 32 * REGBYTES(sp)

	csrr t0, CSR_MSUBM
  	STORE t0,  34*REGBYTES(sp)


	/* Store current stackpointer in task control block (TCB) */
	LOAD	t0, pxCurrentTCB	//pointer
	STORE	sp, 0x0(t0)
.endm


/* Macro for restoring task context */
.macro popREGFILE

	/* Restore registers,
	Skip global pointer because that does not change */
	LOAD	x1, 1 * REGBYTES(sp)
	//LOAD 	x3, 3 * REGBYTES(sp)
	LOAD	x4, 4 * REGBYTES(sp)
	LOAD	x5, 5 * REGBYTES(sp)
	LOAD	x6, 6 * REGBYTES(sp)
	LOAD	x7, 7 * REGBYTES(sp)
	LOAD	x8, 8 * REGBYTES(sp)
	LOAD	x9, 9 * REGBYTES(sp)
	LOAD	x10, 10 * REGBYTES(sp)
	LOAD	x11, 11 * REGBYTES(sp)
	LOAD	x12, 12 * REGBYTES(sp)
	LOAD	x13, 13 * REGBYTES(sp)
	LOAD	x14, 14 * REGBYTES(sp)
	LOAD	x15, 15 * REGBYTES(sp)
#ifndef __riscv_32e
	LOAD	x16, 16 * REGBYTES(sp)
	LOAD	x17, 17 * REGBYTES(sp)
	LOAD	x18, 18 * REGBYTES(sp)
	LOAD	x19, 19 * REGBYTES(sp)
	LOAD	x20, 20 * REGBYTES(sp)
	LOAD	x21, 21 * REGBYTES(sp)
	LOAD	x22, 22 * REGBYTES(sp)
	LOAD	x23, 23 * REGBYTES(sp)
	LOAD	x24, 24 * REGBYTES(sp)
	LOAD	x25, 25 * REGBYTES(sp)
	LOAD	x26, 26 * REGBYTES(sp)
	LOAD	x27, 27 * REGBYTES(sp)
	LOAD	x28, 28 * REGBYTES(sp)
	LOAD	x29, 29 * REGBYTES(sp)
	LOAD	x30, 30 * REGBYTES(sp)
	LOAD	x31, 31 * REGBYTES(sp)
#endif
	addi	sp, sp, REGBYTES * 36
.endm

.macro popREGFILE_Caller

	/* Restore registers,
	Skip global pointer because that does not change */
	LOAD	x1, 1 * REGBYTES(sp)
	////LOAD 	x3, 3 * REGBYTES(sp)
	//LOAD	x4, 4 * REGBYTES(sp)
	LOAD	x5, 5 * REGBYTES(sp)
	LOAD	x6, 6 * REGBYTES(sp)
	LOAD	x7, 7 * REGBYTES(sp)
	//LOAD	x8, 8 * REGBYTES(sp)
	//LOAD	x9, 9 * REGBYTES(sp)
	LOAD	x10, 10 * REGBYTES(sp)
	LOAD	x11, 11 * REGBYTES(sp)
	LOAD	x12, 12 * REGBYTES(sp)
	LOAD	x13, 13 * REGBYTES(sp)
	LOAD	x14, 14 * REGBYTES(sp)
	LOAD	x15, 15 * REGBYTES(sp)
#ifndef __riscv_32e
	LOAD	x16, 16 * REGBYTES(sp)
	LOAD	x17, 17 * REGBYTES(sp)
	//LOAD	x18, 18 * REGBYTES(sp)
	//LOAD	x19, 19 * REGBYTES(sp)
	//LOAD	x20, 20 * REGBYTES(sp)
	//LOAD	x21, 21 * REGBYTES(sp)
	//LOAD	x22, 22 * REGBYTES(sp)
	//LOAD	x23, 23 * REGBYTES(sp)
	//LOAD	x24, 24 * REGBYTES(sp)
	//LOAD	x25, 25 * REGBYTES(sp)
	//LOAD	x26, 26 * REGBYTES(sp)
	//LOAD	x27, 27 * REGBYTES(sp)
	LOAD	x28, 28 * REGBYTES(sp)
	LOAD	x29, 29 * REGBYTES(sp)
	LOAD	x30, 30 * REGBYTES(sp)
	LOAD	x31, 31 * REGBYTES(sp)
#endif
	addi	sp, sp, REGBYTES * 36
.endm
###############################################

###############################################

 /*restore mstatus/mepc and tcb	*/
.macro portRESTORE_CONTEXT_EXCP
	.global	pxCurrentTCB
	/* Load stack pointer from the current TCB */
	LOAD	sp, pxCurrentTCB
	LOAD	sp, 0x0(sp)
	/* Load task program counter */
	LOAD t0,  34*REGBYTES(sp)
  	csrw CSR_MSUBM, t0
	LOAD t0,  33*REGBYTES(sp)
  	csrw CSR_MEPC, t0
  	LOAD t0,  32*REGBYTES(sp)
  	csrw CSR_MSTATUS, t0
.endm



/*saves istatus and tcb	*/
.macro portSAVE_CONTEXT_IRQ
	.global	pxCurrentTCB
	/* Store status */
	csrr t0, CSR_MSTATUS
  	STORE t0,  32*REGBYTES(sp)
	csrr t0,   CSR_MEPC
	STORE t0,  33*REGBYTES(sp)
  	csrr t0,   CSR_MSUBM
  	STORE t0,  34*REGBYTES(sp)
	csrr t0,  CSR_MCAUSE
  	STORE t0,  35*REGBYTES(sp)

	/* Store current stackpointer in task control block (TCB) */
//	LOAD	t0, pxCurrentTCB	//pointer
//	STORE	sp, 0x0(t0)
.endm



 /*restore istatus/ipc and tcb	*/
.macro portRESTORE_CONTEXT_IRQ
	.global	pxCurrentTCB
	/* Load stack pointer from the current TCB */
//	LOAD	sp, pxCurrentTCB
//	LOAD	sp, 0x0(sp)
	#---- Critical section with interrupts disabled -----------------------
   	# Disable interrupts


	LOAD t0,  35*REGBYTES(sp)
  	csrw  CSR_MCAUSE, t0
	LOAD t0,  34*REGBYTES(sp)
  	csrw CSR_MSUBM, t0
	LOAD t0,  33*REGBYTES(sp)
  	csrw CSR_MEPC, t0
  	LOAD t0,  32*REGBYTES(sp)
  	csrw CSR_MSTATUS, t0
.endm

###############################################
# Disable Interrupt
#
.macro DISABLE_MIE
  	csrc CSR_MSTATUS, MSTATUS_MIE
.endm


###############################################

/* Interrupt entry point */
.global trap_entry
.align 6
trap_entry:
	/* Check for interrupt */
	pushREGFILE
	csrr	a0, mcause
    	//Bob: we dont need to check interrupt here because N200 have seperated entry to IRQ
	//blt		a0,zero,interrupt

	/* synchronous interrupt*/
	/* pass mcause in a0 */
	/* pass sp in a1 */
	mv a1,sp
	jal ulSynchTrap
	/*  adjust stack pointer back to where it was prior to ulSynchTrap  */
	mv		sp,a0
	popREGFILE
	mret

###############################################
###############################################

/* async interrupt, this function is called */
//Bob: Rename this to irq_entry to make it override the weak symbol from BSPs entry.S
//interrupt:
.align 2
.global irq_entry
irq_entry: // -------------> This label will be set to MTVT2 register
  	// Allocate the stack space
  	pushREGFILE
	portSAVE_CONTEXT_IRQ

int_loop:             // 3 instructions in pending-interrupt service loop.
  	csrrw ra, CSR_JALMNXTI, ra

  	csrrsi a0, CSR_MNXTI, MSTATUS_MIE   // Claim any pending interrupt at level > mcause.pil
  	bnez a0, int_loop   // Loop to service any interrupt.

  	#---- Critical section with interrupts disabled -----------------------
  	DISABLE_MIE # Disable interrupts

	portRESTORE_CONTEXT_IRQ
  	popREGFILE
  	mret

.align 2
.globl MTIME_HANDLER
MTIME_HANDLER:
  	pushREGFILE
	portSAVE_CONTEXT_IRQ

	csrs CSR_MSTATUS, MSTATUS_MIE
	jal		vPortSysTickHandler
	csrc CSR_MSTATUS, MSTATUS_MIE

	portRESTORE_CONTEXT_IRQ
	popREGFILE

	mret

#if 0
.align 2
.globl MSIP_HANDLER
MSIP_HANDLER:
	pushREGFILE
	portSAVE_CONTEXT_IRQ

	csrs CSR_MSTATUS, MSTATUS_MIE
	jal 	test_handler
	csrc CSR_MSTATUS, MSTATUS_MIE

	portRESTORE_CONTEXT_IRQ
	popREGFILE

	mret
#endif

xPortStartScheduler:
	jal		vPortSetup
	portRESTORE_CONTEXT_EXCP

	popREGFILE

	mret


.align 6
vPortYield:
	mv		sp,a0

	portSAVE_CONTEXT_EXCP

	STORE	a1, 33 * REGBYTES(sp)
	jal		vTaskSwitchContext

	portRESTORE_CONTEXT_EXCP
	popREGFILE

	mret

vPortEndScheduler:
1:
 j 1b



//Bob: we dont need this
//.weak handle_interrupt
//handle_interrupt:
//1:
  j 1b



