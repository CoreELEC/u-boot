#ifndef __IRQ_H_
#define __IRQ_H_


#define CONCAT_STAGE_1(w, x, y, z) w ## x ## y ## z
#define CONCAT2(w, x) CONCAT_STAGE_1(w, x, , )
#define CONCAT3(w, x, y) CONCAT_STAGE_1(w, x, y, )
#define CONCAT4(w, x, y, z) CONCAT_STAGE_1(w, x, y, z)

/* Helper macros to build the IRQ handler and priority struct names */
#define IRQ_HANDLER(irqname) CONCAT3(eclic_irq, irqname, _handler)
/*
 * Macro to connect the interrupt handler "routine" to the irq number "irq" and
 * ensure it is enabled in the interrupt controller with the right priority.
 */
#define DECLARE_IRQ(irq, routine) DECLARE_IRQ_(irq, routine)
#define DECLARE_IRQ_(irq, routine)                    \
	void IRQ_HANDLER(irq)(void)				\
	{							\
		routine();				\
	}

/*IRQ_NUM define list*/
#define IRQ_NUM_MB  35


#endif
