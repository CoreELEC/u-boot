/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef EXTRA_REGISTER_H
#define EXTRA_REGISTER_H

/* macro redefine */
#define SEC_AO_WATCHDOG_CNTL AO_WATCHDOG_CNTL
#define SEC_SYS_CPU_CFG10 SYS_CPU_CFG10

#define AOCPU_IRQ_SEL0 AO_CPU_IRQSEL0
// Bit 30:24-       0   - INTISR03 interrupt source select in int_map
// Bit 22:16-       0   - INTISR02 interrupt source select in int_map
// Bit 14:8 -       0   - INTISR01 interrupt source select in int_map
// Bit  6:0 -       0   - INTISR00 interrupt source select in int_map
#define AOCPU_IRQ_SEL1 AO_CPU_IRQSEL1
// Bit 30:24-       0   - INTISR07 interrupt source select in int_map
// Bit 22:16-       0   - INTISR06 interrupt source select in int_map
// Bit 14:8 -       0   - INTISR05 interrupt source select in int_map
// Bit  6:0 -       0   - INTISR04 interrupt source select in int_map
#define AOCPU_IRQ_SEL2 AO_CPU_IRQSEL2
// Bit 30:24-       0   - INTISR11 interrupt source select in int_map
// Bit 22:16-       0   - INTISR10 interrupt source select in int_map
// Bit 14:8 -       0   - INTISR09 interrupt source select in int_map
// Bit  6:0 -       0   - INTISR08 interrupt source select in int_map
#define AOCPU_IRQ_SEL3 AO_CPU_IRQSEL3
// Bit 30:24-       0   - INTISR15 interrupt source select in int_map
// Bit 22:16-       0   - INTISR14 interrupt source select in int_map
// Bit 14:8 -       0   - INTISR13 interrupt source select in int_map
// Bit  6:0 -       0   - INTISR12 interrupt source select in int_map
#define AOCPU_IRQ_SEL4 AO_CPU_IRQSEL4
// Bit 30:24-       0   - INTISR19 interrupt source select in int_map
// Bit 22:16-       0   - INTISR18 interrupt source select in int_map
// Bit 14:8 -       0   - INTISR17 interrupt source select in int_map
// Bit  6:0 -       0   - INTISR16 interrupt source select in int_map
#define AOCPU_IRQ_SEL5 AO_CPU_IRQSEL5
// Bit 30:24-       0   - INTISR23 interrupt source select in int_map
// Bit 22:16-       0   - INTISR22 interrupt source select in int_map
// Bit 14:8 -       0   - INTISR21 interrupt source select in int_map
// Bit  6:0 -       0   - INTISR20 interrupt source select in int_map
#define AOCPU_IRQ_SEL6 AO_CPU_IRQSEL6
// Bit 30:24-       0   - INTISR27 interrupt source select in int_map
// Bit 22:16-       0   - INTISR26 interrupt source select in int_map
// Bit 14:8 -       0   - INTISR25 interrupt source select in int_map
// Bit  6:0 -       0   - INTISR24 interrupt source select in int_map
#define AOCPU_IRQ_SEL7 AO_CPU_IRQSEL7
// Bit 30:24-       0   - INTISR31 interrupt source select in int_map
// Bit 22:16-       0   - INTISR30 interrupt source select in int_map
// Bit 14:8 -       0   - INTISR29 interrupt source select in int_map
// Bit  6:0 -       0   - INTISR28 interrupt source select in int_map
#endif // EXTRA_REGISTER_H

