/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ENTRY_S
#define ENTRY_S

#include "riscv_encoding.h"
#include "portContext.h"

#if __riscv_xlen == 64
#define SLL32 sllw
#define STORE sd
#define LOAD ld
#define LWU lwu
#define LOG_REGBYTES 3
#else
#define SLL32 sll
#define STORE sw
#define LOAD lw
#define LWU lw
#define LOG_REGBYTES 2
#endif
#define REGBYTES (1 << LOG_REGBYTES)

#if __riscv_flen == 64
#define FPSTORE fsd
#define FPLOAD fld
#define LOG_FPREGBYTES 3
#else
#define FPSTORE fsw
#define FPLOAD flw
#define LOG_FPREGBYTES 2
#endif

.align 2
.extern interrupt_register_nmi
.extern interrupt_register_exception

.macro wr_reg, address, value
    li  t0, \address
    li  t1, \value
    sw  t1, (t0)
.endm

###############################################
###############################################
# Disable Interrupt
#
.macro DISABLE_MIE
  csrc CSR_MSTATUS, MSTATUS_MIE
.endm
###############################################
###############################################
# Enable Interrupt
#
.macro ENABLE_MIE
  csrs CSR_MSTATUS, MSTATUS_MIE  
.endm
###############################################
###############################################
# save exception context
.macro CONTEXT_SAVE_EXCEPTION_CONTEXT
    portcontextSAVE_CONTEXT_INTERNAL
    csrr a0, mcause
    csrr a1, mepc
    addi a1, a1, 4                      /* Synchronous so update exception return address to the instruction after the instruction that generated the exception. */
    store_x a1, 0( sp )                 /* Save updated exception return address. */
    mv a1, sp
    load_x sp, xISRStackTop             /* Switch to ISR stack. */
.endm
###############################################
###############################################
# restore exception context
.macro CONTEXT_RESTORE_CONTEXT
  portcontextRESTORE_CONTEXT
.endm
###############################################
###############################################

  .align 6
  .global exception_entry
exception_entry:
  CONTEXT_SAVE_EXCEPTION_CONTEXT
  call interrupt_register_exception
  CONTEXT_RESTORE_CONTEXT

  mret

###############################################
###############################################

  .align 2
  .global nmi_entry
nmi_entry:
  portcontextSAVE_INTERRUPT_CONTEXT
  call interrupt_register_nmi
  portcontextRESTORE_CONTEXT

  mret

###############################################
###############################################

  .align 2
  .global int_entry
int_entry:
  portcontextSAVE_INTERRUPT_CONTEXT

service_loop:
  // Claim the CLIC to find its pending highest ID
  // if the ID is not 0, then automatically enable the mstatus.MIE, and jump to its vector-entry-label
  // and update the link register
  csrrw ra, CSR_JALMNXTI, ra

  #---- Critical section with interrupts disabled -----------------------
  DISABLE_MIE # Disable interrupts

  portcontextRESTORE_CONTEXT

  // Return to regular code
  mret
  
#endif
