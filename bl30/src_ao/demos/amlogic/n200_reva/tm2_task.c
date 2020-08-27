/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Task scheduling / events module for Chrome EC operating system */

#include "atomic.h"
#include "common.h"
#include "console.h"
#include "cpu.h"
#include "link_defs.h"
#include "panic.h"
#include "task.h"
#include "timer.h"
#include "uart.h"
#include "util.h"
#include "system.h"
#include "riscv_encoding.h"
#include "pic.h"
#include "uart.h"
#include <user_task_config.h>

#define PMP_SEC_SIZE	(0x8000)

extern void serial_put_hex(unsigned long data, unsigned int bitlen);
extern void serial_puts(const char *s);

typedef union {
	struct {
		/*
		 * Note that sp must be the first element in the task struct
		 * for __switchto() to work.
		 */
		uint32_t sp;	/* Saved stack pointer for context switch */
		uint32_t events;	/* Bitmaps of received events */
		uint64_t runtime;	/* Time spent in task */
		uint32_t *stack;	/* Start of stack */
		uint32_t unprvg_flag;	/*Privileged(0)/Unprivileged(1) Thread mode */
	};
} task_;

/* Value to store in unused stack */
#define STACK_UNUSED_VALUE 0xdeadd00d

/* declare task routine prototypes */
#define TASK(n, r, d, s) int r(void *);
void __idle(void);
CONFIG_TASK_LIST CONFIG_TEST_TASK_LIST
#undef TASK
/* Task names for easier debugging */
#define TASK(n, r, d, s)  #n,
static const char *const task_names[] = {
	"<< idle >>",
	CONFIG_TASK_LIST CONFIG_TEST_TASK_LIST
};

#undef TASK

#ifdef CONFIG_TASK_PROFILING
//static uint64_t task_start_time;	/* Time task scheduling started */
static uint64_t exc_start_time;	/* Time of task->exception transition */
static uint64_t exc_end_time;	/* Time of exception->task transition */
static uint64_t exc_total_time;	/* Total time in exceptions */
static uint32_t svc_calls;	/* Number of service calls */
static uint32_t task_switches;	/* Number of times active task changed */
static uint32_t irq_dist[CONFIG_IRQ_COUNT];	/* Distribution of IRQ calls */
#endif

extern void __switchto(task_ *from, task_ *to);
extern int __task_start(void);

#ifndef CONFIG_LOW_POWER_IDLE
/* Idle task.  Executed when no tasks are ready to be scheduled. */
void __idle(void)
{
#ifdef CONFIG_AML_CHIP_M3
	system_ready();
#endif
	while (1) {
	//	serial_puts("idle==================\n");
		/*
		 * Wait for the next irq event.  This stops the CPU clock
		 * (sleep / deep sleep, depending on chip config).
		 */
		asm("wfi");
	}
}
#endif				/* !CONFIG_LOW_POWER_IDLE */

static void task_exit_trap(void)
{
	int i = task_get_current();
	cprints(CC_TASK, "Task %d (%s) exited!", i, task_names[i]);
	/* Exited tasks simply sleep forever */
	while (1)
		task_wait_event(-1);
}

/* Startup parameters for all tasks. */
#define TASK(n, r, d, s)  {	\
	.a0 = (uint32_t)d,	\
	.pc = (uint32_t)r,	\
	.stack_size = s,	\
},
static const struct {
	uint32_t a0;
	uint32_t pc;
	uint16_t stack_size;
} tasks_init[] = {
	TASK(IDLE, __idle, 0, IDLE_TASK_STACK_SIZE)
	CONFIG_TASK_LIST CONFIG_TEST_TASK_LIST
};

#undef TASK

/* Contexts for all tasks */
static task_ tasks[TASK_ID_COUNT];
/* Sanity checks about static task invariants */
BUILD_ASSERT(TASK_ID_COUNT <= sizeof(unsigned) * 8);
BUILD_ASSERT(TASK_ID_COUNT < (1 << (sizeof(task_id_t) * 8)));

/* Stacks for all tasks */
#define TASK(n, r, d, s)  + s
#if 1
uint8_t *task_stacks = (uint8_t *)CONFIG_SYS_STACK_BASE;
#else
uint8_t task_stacks[0 TASK(IDLE, __idle, 0, IDLE_TASK_STACK_SIZE)
		    CONFIG_TASK_LIST CONFIG_TEST_TASK_LIST] __aligned(8);
#endif
#undef TASK

/* Reserve space to discard context on first context switch. */
uint32_t scratchpad[100];

task_ *current_task = (task_ *) scratchpad;

/*
 * Should IRQs chain to svc_handler()?  This should be set if either of the
 * following is true:
 *
 * 1) Task scheduling has started, and task profiling is enabled.  Task
 * profiling does its tracking in svc_handler().
 *
 * 2) An event was set by an interrupt; this could result in a higher-priority
 * task unblocking.  After checking for a task switch, svc_handler() will clear
 * the flag (unless profiling is also enabled; then the flag remains set).
 */
static int need_resched_or_profiling;

/*
 * Bitmap of all tasks ready to be run.
 *
 * Currently all tasks are enabled at startup.
 */
static uint32_t tasks_ready = (1 << TASK_ID_COUNT) - 1;

int start_called;	/* Has task swapping started */

static void pmp_config0(void)
{
	uint32_t  bgadr;
	bgadr = (PMP_NAPOT | PMP_X | PMP_R | PMP_W) << 16;	/*pmp cfg2*/
	bgadr = ((PMP_NAPOT) << 8) | bgadr;	/*pmp cfg2 + pmp cfg1*/
	bgadr = (PMP_NAPOT) | bgadr;  /*pmp cfg2 + pmp cfg1 + pmp cfg0*/
	write_csr_pmpcfg0(bgadr);
	write_csr_pmpaddr0((CONFIG_RAM_BASE >> 2) | 0xfff); /*32k*/
	write_csr_pmpaddr1(((CONFIG_RAM_BASE + PMP_SEC_SIZE) >> 2) | 0x7ff); /*16k*/
	write_csr_pmpaddr2(0x7fffffff);  /*4G*/
}

static inline task_ *__task_id_to_ptr(task_id_t id)
{
	return tasks + id;
}

void interrupt_disable(void)
{
	clear_csr(mstatus,MSTATUS_MIE);
}

void interrupt_enable(void)
{
	set_csr(mstatus, MSTATUS_MIE);
}

unsigned long interrupt_status_get(void)
{
	return read_csr(mstatus) >> 0x3;
}

inline int in_interrupt_context(void)
{
	return ((read_csr_msubmode & 0x3) == 0x3);
}
#if 0
inline int get_interrupt_context(void)
{
	int ret;
 asm("mrs %0, ipsr\n":"=r"(ret));
				/* read exception number */
	return ret & 0x1ff;	/* exception bits are the 9 LSB */
}
#endif
task_id_t task_get_current(void)
{
	return current_task - tasks;
}

uint32_t *task_get_event_bitmap(task_id_t tskid)
{
	task_ *tsk = __task_id_to_ptr(tskid);
	return &tsk->events;
}

int task_start_called(void)
{
	return start_called;
}


__attribute__((weak)) uintptr_t handle_trap_panic(void)
{
	uint32_t mstatus_mps_bits = ((read_csr(mstatus) & MSTATUS_MPS) >> MSTATUS_MPS_LSB);
	cprints(CC_SYSTEM, "In trap handler, msubmode = 0x%x\n", read_csr_msubmode);
	cprints(CC_SYSTEM, "In trap handler, mstatus.MPS = 0x%x\n", mstatus_mps_bits);
	cprints(CC_SYSTEM, "In trap handler, mcause = %d\n", read_csr(mcause));
	cprints(CC_SYSTEM, "In trap handler, mepc = 0x%x\n", read_csr(mepc));
	cprints(CC_SYSTEM, "In trap handler, mtval = 0x%x\n", read_csr(mbadaddr));
	if (mstatus_mps_bits == 0x1) {
		cprints(CC_SYSTEM, "Double-Exception-fault!\n");
	} else if (mstatus_mps_bits == 0x2){
		cprints(CC_SYSTEM, "Exception from previous NMI mode!\n");
	} else if (mstatus_mps_bits == 0x3){
		cprints(CC_SYSTEM, "Exception from previous IRQ mode!\n");
	}
	while (1);
	return 0;
}

void software_panic(uint32_t reason, uint32_t info)
{
	cprints(CC_SYSTEM, "Panic: reason=0x%x line=%d\n", reason, info);
	handle_trap_panic();
}


/**
 * Scheduling system call
 */

extern int __clzsi2(int x);
void cnt_sw_handler(int desched, task_id_t resched)
{
	//unsigned int i;
	task_ *current, *next;

	if (!in_interrupt_context()) {
		if ((read_csr(mcause) != 0x8) && (read_csr(mcause) != 0xB))
			handle_trap_panic();
	}

	/*
	 * Push the priority to -1 until the return, to avoid being
	 * interrupted.
	 */

	current = current_task;

#ifdef CONFIG_DEBUG_STACK_OVERFLOW
	if (*current->stack != STACK_UNUSED_VALUE) {
		panic_printf("\n\nStack overflow in %s task!\n",
			     task_names[current - tasks]);
#ifdef CONFIG_SOFTWARE_PANIC
		handle_trap_panic();
#endif
	}
#endif

	if ((desched == 1 && !current->events) | (desched == 2)) {
		/*
		 * Remove our own ready bit (current - tasks is same as
		 * task_get_current())
		 */
		tasks_ready &= ~(1 << (current - tasks));
	}
	tasks_ready |= 1 << resched;

	ASSERT(tasks_ready);
	next = __task_id_to_ptr(31 - __clzsi2(tasks_ready));

#ifdef CONFIG_TASK_PROFILING
	/* Track time in interrupts */
	t = get_time().val;
	exc_total_time += (t - exc_start_time);

	/*
	 * Bill the current task for time between the end of the last interrupt
	 * and the start of this one.
	 */
	current->runtime += (exc_start_time - exc_end_time);
	exc_end_time = t;
#else
	/*
	 * Don't chain here from interrupts until the next time an interrupt
	 * sets an event.
	 */
	need_resched_or_profiling = 0;
#endif
//	serial_puts("111111\n");
//	serial_put_hex((unsigned long)next,32);
//	serial_puts("\n");

/*
	serial_puts("\ncurrent statck:\n");
	for (i=0;i<34;i++) {
		serial_put_hex(*(unsigned int *)(current_task->sp+i*4),32);
		serial_puts("\n");
		}
*/

	/* Nothing to do */
	if (next == current)
		return;
//	serial_puts("22222\n");

	/* Switch to new task */
#ifdef CONFIG_TASK_PROFILING
	task_switches++;
#endif
	current_task = next;
//	serial_puts("33333\n");
/*
	serial_puts("\nnext statck:\n");
	for (i=0;i<34;i++) {
		serial_put_hex(*(unsigned int *)(current_task->sp+i*4),32);
		serial_puts("\n");
		}
*/
	/*If call svc_handler in interrupt contex, will do switch in irq_entry*/
	if (!in_interrupt_context()) {
//		serial_puts("4444\n");
		__switchto(current, next);
	}
}

void __schedule(int desched, int resched)
{
	register int p0 asm("a0") = desched;
	register int p1 asm("a1") = resched;

	asm("ecall" :  : "r"(p0), "r"(p1));
}

#ifdef CONFIG_TASK_PROFILING
void task_start_irq_handler(void *excep_return)
{
	/*
	 * Get time before checking depth, in case this handler is
	 * pre-empted.
	 */
	uint64_t t = get_time().val;
	int irq = get_interrupt_context() - 16;

	/*
	 * Track IRQ distribution.  No need for atomic add, because an IRQ
	 * can't pre-empt itself.
	 */
	if (irq < ARRAY_SIZE(irq_dist))
		irq_dist[irq]++;

	/*
	 * Continue iff a rescheduling event happened or profiling is active,
	 * and we are not called from another exception (this must match the
	 * logic for when we chain to svc_handler() below).
	 */
	if (!need_resched_or_profiling
	    || (((uint32_t) excep_return & 0xf) == 1))
		return;

	exc_start_time = t;
}
#endif

void task_resched_if_needed(void *excep_return)
{
	/*
	 * Continue iff a rescheduling event happened or profiling is active,
	 * and we are not called from another exception.
	 */
	if (!need_resched_or_profiling
	    || (((uint32_t) excep_return & 0xf) == 1))
		return;

	__schedule(0, 0);
}

static uint32_t __wait_evt(int timeout_us, task_id_t resched)
{
	task_ *tsk = current_task;
	task_id_t me = tsk - tasks;
	uint32_t evt;
	int ret __attribute__ ((unused));

	ASSERT(!in_interrupt_context());

	if (timeout_us > 0) {
		timestamp_t deadline = get_time();
		deadline.val += timeout_us;

		ret = timer_arm(deadline, me);
		ASSERT(ret == EC_SUCCESS);
	}
	while (!(evt = atomic_read_clear(&tsk->events))) {
		/* Remove ourself and get the next task in the scheduler */
		__schedule(1, resched);
		resched = TASK_ID_IDLE;
	}
	if (timeout_us > 0)
		timer_cancel(me);
	return evt;
}

uint32_t task_set_event(task_id_t tskid, uint32_t event, int wait)
{
	task_ *receiver = __task_id_to_ptr(tskid);
	ASSERT(receiver);

	/* Set the event bit in the receiver message bitmap */
	atomic_or(&receiver->events, event);

	/* Re-schedule if priorities have changed */
	if (in_interrupt_context()) {
		/* The receiver might run again */
		atomic_or(&tasks_ready, 1 << tskid);
#ifndef CONFIG_TASK_PROFILING
		if (start_called)
			need_resched_or_profiling = 1;
#endif
	} else {
		if (wait)
			return __wait_evt(-1, tskid);
		else
			__schedule(0, tskid);
	}

	return 0;
}

uint32_t task_wait_event(int timeout_us)
{
	return __wait_evt(timeout_us, TASK_ID_IDLE);
}

uint32_t task_wait_event_mask(uint32_t event_mask, int timeout_us)
{
	uint64_t deadline = get_time().val + timeout_us;
	uint32_t events = 0;
	int time_remaining_us = timeout_us;

	/* Add the timer event to the mask so we can indicate a timeout */
	event_mask |= TASK_EVENT_TIMER;

	while (!(events & event_mask)) {
		/* Collect events to re-post later */
		events |= __wait_evt(time_remaining_us, TASK_ID_IDLE);

		time_remaining_us = deadline - get_time().val;
		if (timeout_us > 0 && time_remaining_us <= 0) {
			/* Ensure we return a TIMER event if we timeout */
			events |= TASK_EVENT_TIMER;
			break;
		}
	}

	/* Re-post any other events collected */
	if (events & ~event_mask)
		atomic_or(task_get_event_bitmap(task_get_current()),
			  events & ~event_mask);

	return events & event_mask;
}


// Note that there are no assertions or bounds checking on these
// parameter values.

void pic_set_threshold (uint32_t threshold)
{
	writel(threshold, PIC_CTRL_ADDR + PIC_THRESHOLD_OFFSET);
 }

void pic_enable_interrupt (uint32_t source)
{
	//Source number divide 32 and then multip 4 (bytes)
	volatile uint32_t * current_ptr;
	current_ptr = (volatile uint32_t *) (PIC_CTRL_ADDR + PIC_ENABLE_OFFSET
		                                  + ((source >> 3) & (~0x3)));

	writel(readl(current_ptr) | ( 1 << (source & 0x1f)), current_ptr);
}

void pic_disable_interrupt (uint32_t source)
{
	//Source number divide 32 and then multip 4 (bytes)
	volatile uint32_t * current_ptr;
	current_ptr = (volatile uint32_t *) (PIC_CTRL_ADDR + PIC_ENABLE_OFFSET
		                                  + ((source >> 3) & (~0x3)));

	writel(readl(current_ptr) & ~( 1 << (source & 0x1f)), current_ptr);
}

void pic_set_priority (uint32_t source, uint32_t priority)
{
	if (PIC_NUM_PRIORITIES > 0) {
		writel(priority, PIC_CTRL_ADDR + PIC_PRIORITY_OFFSET
		+ (source << PIC_PRIORITY_SHIFT_PER_SOURCE));
	}
}

uint32_t pic_claim_interrupt(void)
{
	return readl(PIC_CTRL_ADDR + PIC_CLAIM_OFFSET);
}

uint32_t pic_check_eip(void)
{
	return readl(PIC_CTRL_ADDR + PIC_EIP_OFFSET);
}

void pic_complete_interrupt(uint32_t source)
{
	writel(source, PIC_CTRL_ADDR + PIC_CLAIM_OFFSET);
}

void task_enable_irq(int irq)
{
	pic_enable_interrupt((unsigned int)irq);
}

void task_disable_irq(int irq)
{
	pic_disable_interrupt((unsigned int)irq);
}

static unsigned int irq_setting[IRQ_EN_REG_NUM];
/*N205 does not support clear pending irq.*
 *Need use work around to clear pending:  *
 *1. disable irq (MIE)                    *
 *2. store and disable current enable irq *
 *3. enable target irq                    *
 *4. claim and complete target irq        *
 *5. disable target irq                   *
 *6. restore current irq enable setting   *
 *7. enable irq (MIE)
*/
void task_clear_pending_irq(int irq)
{
	unsigned int i;
	volatile uint32_t * current_ptr;
	unsigned long irq_status;

	irq_status = interrupt_status_get();
	if (irq_status)
		interrupt_disable();

	for (i = 0; i < IRQ_EN_REG_NUM; i++)
	{
		current_ptr =
			(volatile uint32_t *)(PIC_CTRL_ADDR + PIC_ENABLE_OFFSET + i*4);
		irq_setting[i] =
			readl(current_ptr);
		writel(0, current_ptr);
	}
	task_enable_irq(irq);
	i = pic_claim_interrupt();
	if (i)
		pic_complete_interrupt(i);
	task_disable_irq(irq);

	for (i = 0; i < IRQ_EN_REG_NUM; i++)
	{
		current_ptr =
			(volatile uint32_t *)(PIC_CTRL_ADDR + PIC_ENABLE_OFFSET + i*4);
		writel(irq_setting[i], current_ptr);
	}

	if (irq_status)
		interrupt_enable();
}

void task_trigger_irq(int irq)
{
//	serial_puts("trigger...\n\n\n ");
	/*use 1 us timer to trigger a timer irq*/
	//__hw_clock_event_set(1);
	//Use soft interrupt to trigger a timer event interrupt
	writel(1, TMR_CTRL_ADDR + TMR_MSIP);
}

/*
 * Initialize IRQs in the NVIC and set their priorities as defined by the
 * DECLARE_IRQ statements.
 */
extern void trap_entry(void);
extern void nmi_entry(void);
extern void irq_entry(void);
void no_interrupt_handler (void) {};

__attribute__((weak)) function_ptr_t pic_interrupt_handlers[CONFIG_PIC_NUM_INTERRUPTS];

static void __init_irqs(void)
{
	/* Get the IRQ priorities section from the linker */
	int exc_calls = __irqprio_end - __irqprio;
	int i;

	/* Set priorities */
	for (i = 0; i < exc_calls; i++) {
		pic_interrupt_handlers[__irqprio[i].irq] = (function_ptr_t)__irqprio[i].routine;
		pic_set_priority(__irqprio[i].irq, __irqprio[i].priority);
	}
}

void _init_irqs(void)
{
	// Disable the machine & timer interrupts until setup is done.
	//clear_csr(mie, MIP_MEIP);
	//clear_csr(mie, MIP_MTIP);
	clear_csr(mstatus, MSTATUS_MIE);

	for (int i = 0; i < PIC_NUM_INTERRUPTS; i++) {
		pic_interrupt_handlers[i] = no_interrupt_handler;
	}
	__init_irqs();
	//set_csr(mie, MIP_MEIP);
	set_csr(mstatus, MSTATUS_MIE);
}
/*
void enable_interrupt(uint32_t int_num, uint32_t int_priority, function_ptr_t handler) {
    pic_interrupt_handlers[int_num] = handler;
    pic_set_priority(int_num, int_priority);
    pic_enable_interrupt (int_num);
}
*/
static void init_irqs(void)
{
	/* Get the IRQ priorities section from the linker */
	//int exc_calls = __irqprio_end - __irqprio;

	write_csr(mtvec, &trap_entry);
	   // The N200 self-defined CSR (not standard RISC-V CSR) must use this function style
	write_csr_mivec(&irq_entry);
	write_csr_mnvec(&nmi_entry);

	_init_irqs();
}

/*Entry Point for PIC Interrupt Handler*/
__attribute__((weak)) uint32_t handle_irq(void)
{
	unsigned int int_num;

	int_num = pic_claim_interrupt();

//	serial_puts("interrupt= ");
//	serial_put_hex(int_num,32);
//	serial_puts("\n");

	// Enable interrupts to allow interrupt preempt based on priority
	//set_csr(mstatus, MSTATUS_MIE);
	pic_interrupt_handlers[int_num]();
	// Disable interrupts
	//clear_csr(mstatus, MSTATUS_MIE);
    pic_complete_interrupt(int_num);

	if (start_called)
		cnt_sw_handler(0, 0);

	return int_num;
}

__attribute__((weak)) uintptr_t handle_nmi(void)
{
	while (1)
	{
		panic_printf("nmi!!\n");
	}
	return 0;
}

#if 0
void mutex_lock(struct mutex *mtx)
{
}

void mutex_unlock(struct mutex *mtx)
{
}
#endif



unsigned long MSTATUS_INIT = (MSTATUS_MPP | MSTATUS_MPIE);

void task_pre_init(void)
{
	uint32_t *stack_next = (uint32_t *) task_stacks;
	int i;
	int usr_i = 0;

	/* Fill the task memory with initial values */
	for (i = 0; i < TASK_ID_COUNT; i++) {
		uint32_t *sp;
		/* Stack size in words */
		uint32_t ssize = tasks_init[i].stack_size / 4;

		tasks[i].stack = stack_next;

		if (!ssize) {
			sp = (uint32_t *) (CONFIG_BL301_STACK_BASE + (usr_i + 1)*CONFIG_USER_TASK_STACK_SIZE) - 34;
			tasks[i].stack = (uint32_t *) (CONFIG_BL301_STACK_BASE + usr_i*CONFIG_USER_TASK_STACK_SIZE);
			usr_i++;
		} else
			sp = stack_next + ssize - 34;

		tasks[i].sp = (uint32_t) sp;

		if (!ssize)
			tasks[i].unprvg_flag = 1;
		else
			tasks[i].unprvg_flag = 0;

		/* Initial context on stack (see __switchto()) */
		sp[10] = tasks_init[i].a0;	/* r0 */
		sp[1] = (uint32_t) task_exit_trap;	/* lr */

		if (i == TASK_ID_USERSECURETASK)
			sp[33] = USER_SECURE_TASK_ENTRY_ADDR;
		else if (i == TASK_ID_USERHIGHTASK)
			sp[33] = USER_HIGH_TASK_ENTRY_ADDR;
		else if (i == TASK_ID_USERLOWTASK)
			sp[33] = USER_LOW_TASK_ENTRY_ADDR;
		else
			sp[33] = tasks_init[i].pc;		/* pc */
		sp[32] = MSTATUS_INIT;	/* mstatus */

		/* Fill unused stack; also used to detect stack overflow. */
		for (sp = tasks[i].stack; sp < (uint32_t *) tasks[i].sp; sp++)
			*sp = STACK_UNUSED_VALUE;
#if 0
		serial_put_hex(sizeof(tasks)/sizeof(task_),32);
		serial_puts("task_init:\n");
		serial_puts(task_names[i]);
		serial_puts("\n");
		serial_puts("task_addr=\n");
		serial_put_hex((unsigned long)&tasks[i].sp,32);
		serial_puts("\n");
		serial_puts("start of stack:\n");
		serial_put_hex((unsigned long)tasks[i].stack,32);
#endif
		stack_next += ssize;
#if 0
		serial_puts("stack:\n");
		serial_put_hex(*(unsigned int *)(tasks[i].sp+1*4),32);
		serial_puts("\n");
		serial_put_hex(*(unsigned int *)(tasks[i].sp+10*4),32);
		serial_puts("\n");
		serial_put_hex(*(unsigned int *)(tasks[i].sp+32*4),32);
		serial_puts("\n");
		serial_put_hex(*(unsigned int *)(tasks[i].sp+33*4),32);
		serial_puts("\n");
#endif
	}

	/*
	 * Fill in guard value in scratchpad to prevent stack overflow
	 * detection failure on the first context switch.  This works because
	 * the first word in the scratchpad is where the switcher will store
	 * sp, so it's ok to blow away.
	 */
	((task_ *) scratchpad)->stack = &((task_ *) scratchpad)->events;
	((task_ *) scratchpad)->events = STACK_UNUSED_VALUE;

	/* Initialize IRQs */
	init_irqs();
	pmp_config0();
}

int task_start(void)
{
	start_called = 1;
	/*Has a risk here. After enable star_called, interrupt may happen here*/
	return __task_start();
}
