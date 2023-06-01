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
#define CONCAT2(w, x) w##x
#define CONCAT3(w, x, y) w##x##y
#define CONCAT4(w, x, y, z) w##x##y##z

/* Helper macros to build the IRQ handler and priority struct names */
#define IRQ_HANDLER(irqname) CONCAT3(eclic_irq, irqname, _handler)
/*
 * Macro to connect the interrupt handler "routine" to the irq number "irq" and
 * ensure it is enabled in the interrupt controller with the right priority.
 */
#define DECLARE_IRQ(irq, routine) DECLARE_IRQ_(irq, routine)
#define DECLARE_IRQ_(irq, routine)                                                                 \
	void IRQ_HANDLER(irq)(void)                                                                \
	{                                                                                          \
		routine();                                                                         \
	}

/*IRQ_NUM define list*/
#define IRQ_NUM_MAX  127

/*You can add other interrupts num here 0~127*/

/* use for ir */
#define IRQ_NUM_IRIN 4

/* APCore GIC_OUTx */
#define IRQ_NUM_OUT_3 (28 + 13 + 3)
#define IRQ_NUM_OUT_2 (28 + 12 + 3)
#define IRQ_NUM_OUT_1 (28 + 11 + 3)
#define IRQ_NUM_OUT_0 (28 + 10 + 3)

/*wol*/
#define IRQ_ETH_PMT_NUM 73

/* SOC AO Domain timerA~timerC */
#define IRQ_NUM_AO_TIMERA 12
#define IRQ_NUM_AO_TIMERB 13
#define IRQ_NUM_AO_TIMERC 0

#define IRQ_NUM_TIMER IRQ_NUM_AO_TIMERA
#define IRQ_TIMER_PROI 8

#endif
