/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "hwspinlock_bakery.h"
#include "register.h"
#include "mailbox-api.h"
#include "common.h"
#include "string.h"

#ifndef writel32
#define writel32(val, reg)      ((*((volatile uint32_t *)((uintptr_t)(reg)))) =  (val))
#define readl32(reg)            (*((volatile uint32_t *)((uintptr_t)(reg))))
#endif
#ifndef PRINT_DBG
#define PRINT_DBG(...)          printf(__VA_ARGS__)
#define PRINT_ERR(...)          printf(__VA_ARGS__)
#define PRINT(...)              printf(__VA_ARGS__)
#endif

SemaphoreHandle_t xBakLockSemp[HWSPINLOCK_NUMS];
struct hwspinlock_prop {
	uint32_t xAddr;
	uint32_t xCpus;
};
struct hwspinlock_prop prop;

void __assert(const char *function, const char *file, uint32_t line,
		const char *assertion)
{
	PRINT_ERR("[%s] ASSERT: %s <%d> : %s\n", SPINLOCK_TAG, function, line, assertion);
	while (1)
		;
}

#define	assert(e)	((e) ? (void)0 : __assert(__func__, __FILE__, \
			    __LINE__, #e))

#define assert_hwspinlock_entry_valid(entry, hwspinlock) do {	\
	assert(hwspinlock);					\
	assert(entry < HWSPINLOCK_MAX_CPUS);		\
} while (0)

/* Obtain a ticket for a given CPU */
static uint32_t xHwspin_get_ticket(struct hwspinlock_lock *hwspinlock, uint32_t me)
{
	uint32_t my_ticket, their_ticket;
	uint32_t they;

	assert(!hwspinlock_ticket_number(hwspinlock->lock_data[me]));

	my_ticket = 0;
	hwspinlock->lock_data[me] = make_hwspinlock_data(CHOOSING_TICKET, my_ticket);
	for (they = 0; they < HWSPINLOCK_MAX_CPUS; they++) {
		if (me == they || hwspinlock->lock_data[they] == HWSPINLOCK_DEFAULT_VALUE)
			continue;
		their_ticket = hwspinlock_ticket_number(hwspinlock->lock_data[they]);
		if (their_ticket > my_ticket)
			my_ticket = their_ticket;
	}

	/*
	 * Compute ticket; then signal to other contenders waiting for us to
	 * finish calculating our ticket value that we're done
	 */
	++my_ticket;
	hwspinlock->lock_data[me] = make_hwspinlock_data(CHOSEN_TICKET, my_ticket);

	return my_ticket;
}

void vBakspinLock_get(struct hwspinlock_lock *hwspinlock, uint32_t id)
{
	uint32_t they, me;
	uint32_t my_ticket, my_prio, their_ticket;
	uint32_t their_hwspinlock_data;
	uint32_t spinlock_ticket = 0;
	BaseType_t ret;

	me = plat_my_core_pos();

	assert_hwspinlock_entry_valid(me, hwspinlock);
	ret = xSemaphoreTake(xBakLockSemp[id], portMAX_DELAY);
	if (ret == pdFALSE) {
		PRINT_ERR("[%s]: Async get semaphore fail\n", SPINLOCK_TAG);
		configASSERT(ret);
	}

	/* Get a ticket */
	my_ticket = xHwspin_get_ticket(hwspinlock, me);

	/*
	 * Now that we got our ticket, compute our priority value, then compare
	 * with that of others, and proceed to acquire the lock
	 */
	my_prio = PRIORITY(my_ticket, me);
	for (they = 0; they < HWSPINLOCK_MAX_CPUS; they++) {
		if (me == they || hwspinlock->lock_data[they] == HWSPINLOCK_DEFAULT_VALUE)
			continue;

		/* Wait for the contender to get their ticket */
		do {
			//printf("Choosing %d %d\n", they, hwspinlock->lock_data[they]);
			their_hwspinlock_data = hwspinlock->lock_data[they];
		} while (hwspinlock_is_choosing(their_hwspinlock_data));

		/*
		 * If the other party is a contender, they'll have non-zero
		 * (valid) ticket value. If they do, compare priorities
		 */
		their_ticket = hwspinlock_ticket_number(their_hwspinlock_data);
		if (their_ticket && (PRIORITY(their_ticket, they) < my_prio)) {
			/*
			 * They have higher priority (lower value). Wait for
			 * their ticket value to change (either release the lock
			 * to have it dropped to 0; or drop and probably content
			 * again for the same lock to have an even higher value)
			 */
			do {

			} while (their_ticket ==
				hwspinlock_ticket_number(hwspinlock->lock_data[they]));
		}
	}
	/* Lock acquired */
}


/* Release the lock and signal contenders
 * This function should be avoid called in interrupt context, or caused unknown issue
 */
void vBakspinLock_release(struct hwspinlock_lock *hwspinlock, uint32_t id)
{
	uint32_t me = plat_my_core_pos();

	assert_hwspinlock_entry_valid(me, hwspinlock);
	assert(hwspinlock_ticket_number(hwspinlock->lock_data[me]));

	/*
	 * Release lock by resetting ticket. Then signal other
	 * waiting contenders
	 */
	hwspinlock->lock_data[me] = 0;

	__asm__ volatile ("" : : : "memory");

	xSemaphoreGive(xBakLockSemp[id]);
}

void vBakspinLock_init(struct hwspinlock_lock **hwspinlock, uint32_t id,
		      void *hwspinlock_addr)
{
	uint32_t me = plat_my_core_pos();
	uint32_t xAddrOff = id * sizeof(struct hwspinlock_lock);

	if (!hwspinlock_addr)
		hwspinlock_addr = (char *)prop.xAddr + xAddrOff;
	*hwspinlock = hwspinlock_addr;
	PRINT_DBG("hwspinlock addr %p\n", hwspinlock_addr);

	if ((*hwspinlock) && (*hwspinlock)->lock_data[me] == HWSPINLOCK_DEFAULT_VALUE)
		(*hwspinlock)->lock_data[me] = 0;
}

void vBakLockInit(uint32_t xAddr, uint32_t xCpus)
{
	int ret;
	int idx = 0;

	for (idx = 0; idx < HWSPINLOCK_NUMS; idx++) {
		xBakLockSemp[idx] = xSemaphoreCreateBinary();
		configASSERT(xBakLockSemp[idx]);
		xSemaphoreGive(xBakLockSemp[idx]);
	}
	prop.xAddr = xAddr;
	prop.xCpus = xCpus;
	PRINT_ERR("hwspinlock init %p\n", xAddr);

	//init data
	for (idx = 0; idx < 32; idx++)
		REG32(xAddr + 4*idx) = HWSPINLOCK_DEFAULT_VALUE;
}
