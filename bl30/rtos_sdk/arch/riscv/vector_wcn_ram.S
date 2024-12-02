/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

.section .ram.vector
#include "irq_handler.h"

.global vector_ram_base
vector_ram_base:
    .word  irq0_handler
    .word  irq1_handler
    .word  irq2_handler
    .word  irq3_handler
    .word  irq4_handler
    .word  irq5_handler
    .word  irq6_handler
    .word  irq7_handler
    .word  irq8_handler
    .word  irq9_handler

    .word  irq10_handler
    .word  irq11_handler
    .word  irq12_handler
    .word  irq13_handler
    .word  irq14_handler
    .word  irq15_handler
    .word  irq16_handler
    .word  irq17_handler
    .word  irq18_handler
    .word  irq19_handler

    .word  irq20_handler
    .word  irq21_handler
    .word  irq22_handler
    .word  irq23_handler
    .word  irq24_handler
    .word  irq25_handler
    .word  irq26_handler
    .word  irq27_handler
    .word  irq28_handler
    .word  irq29_handler

    .word  irq30_handler
    .word  irq31_handler
    .word  irq32_handler
    .word  irq33_handler
    .word  irq34_handler
    .word  irq35_handler
    .word  irq36_handler
    .word  irq37_handler
    .word  irq38_handler
    .word  irq39_handler

    .word  irq40_handler
    .word  irq41_handler
    .word  irq42_handler
    .word  irq43_handler
    .word  irq44_handler
    .word  irq45_handler
    .word  irq46_handler
    .word  irq47_handler
    .word  irq48_handler
    .word  irq49_handler

    .word  irq50_handler
    .word  irq51_handler

