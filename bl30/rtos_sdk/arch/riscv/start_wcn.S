/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "riscv_encoding.h"

    .section .text.startup
    .extern  main
    .extern vector_base

    .equ  mtvec, 0x305
    .equ  mtvt, 0x307
    .equ  mtvt2, 0x7ec
    .equ  mnvec, 0x7c3

    .global  _start
    .type _start,@function

_start:
    csrrci x0, mstatus, 0x8  //Disable the interrupt globally

    li x1, 0
    li x2, 0
    li x3, 0
    li x4, 0
    li x5, 0
    li x6, 0
    li x7, 0
    li x8, 0
    li x9, 0
    li x10, 0
    li x11, 0
    li x12, 0
    li x13, 0
    li x14, 0
    li x15, 0
    li x16, 0
    li x17, 0
    li x18, 0
    li x19, 0
    li x20, 0
    li x21, 0
    li x22, 0
    li x23, 0
    li x24, 0
    li x25, 0
    li x26, 0
    li x27, 0
    li x28, 0
    li x29, 0
    li x30, 0
    li x31, 0

.option push
.option norelax
    la gp, __global_pointer$
.option pop
    //la sp, _sp
    li sp, 0x0080c000

    // set exception_entry for exception process
    la t0, exception_entry
    csrw mtvec, t0

    // set nmi_entry for non-mask interrupt process
    la t0, nmi_entry
    csrw mnvec, t0

    /* initial the mtvt*/
    la t0, vector_base
    csrw CSR_MTVT, t0

    // set int_entry for peripheral interrupt process
    la t0, int_entry
    csrw mtvt2, t0
    csrs mtvt2, 0x1

    /* argc = argv = 0 */
    li a0, 0
    li a1, 0
    call main
1:
    wfi
    j 1b	/* just in case of pending imprecise data abort */
