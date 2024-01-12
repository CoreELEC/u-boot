/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __IRQ_H_
#define __IRQ_H_

extern void eclic_irq20_handler(void);
extern void eclic_irq21_handler(void);
extern void eclic_irq22_handler(void);
extern void eclic_irq23_handler(void);
extern void eclic_irq24_handler(void);

extern void eclic_irq50_handler(void);

#define CONCAT_STAGE_1(w, x, y, z) w##x##y##z
#define CONCAT2(w, x) w##x
#define CONCAT3(w, x, y) w##x##y
#define CONCAT4(w, x, y, z) CONCAT_STAGE_1(w, x, y, z)

/* Helper macros to build the IRQ handler and priority struct names */
#define IRQ_HANDLER(irqname) CONCAT3(eclic_irq, irqname, _handler)

#define DECLARE_IRQ(irq, routine)

/*IRQ_NUM define list*/
#define IRQ_NUM_MAX  255
#define IRQ_NUM_MB_0 50
#define IRQ_NUM_MB_1 49
#define IRQ_NUM_MB_2 48
#define IRQ_NUM_MB_3 47
#define IRQ_NUM_MB_4 46

/*You can add other interrupts num here 46~19*/

/* use for ir */
#define IRQ_NUM_IRIN 22

/* uart */
#define IRQ_NUM_AO_UART_C 138

/* cec */
#define IRQ_NUM_CECA 40
#define IRQ_NUM_CECB 41

/*wol*/
#define IRQ_ETH_PMT_NUM 76

/* timerA~timerJ */
#define IRQ_NUM_TIMERA 0
#define IRQ_NUM_TIMERB 1
#define IRQ_NUM_TIMERC 2
#define IRQ_NUM_TIMERD 3
#define IRQ_NUM_TIMERG 4
#define IRQ_NUM_TIMERH 5
#define IRQ_NUM_TIMERI 6
#define IRQ_NUM_TIMERJ 7

#define IRQ_NUM_TIMER IRQ_NUM_TIMERJ
#define IRQ_TIMER_PROI 8

#endif
