/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __HWSPINLOCK_H__
#define __HWSPINLOCK_H__

#include "hwspinlock_config.h"

#ifdef M4A
#define CORE_NUMBER	1
#define SPINLOCK_TAG	"M4A"
#elif defined(M4B)
#define CORE_NUMBER	2
#define SPINLOCK_TAG	"M4B"
#elif defined(HIFIA)
#define CORE_NUMBER	3
#define SPINLOCK_TAG	"HIFIA"
#elif defined(HIFIB)
#define CORE_NUMBER	4
#define SPINLOCK_TAG	"HIFIB"
#endif

#define HWSPINLOCK_DEFAULT_VALUE	0xa5a5a5a5
#define HWSPINLOCK_MAX_CPUS	6 /*ARMV8 & M4*/

static inline uint32_t plat_my_core_pos(void)
{
	return CORE_NUMBER;
}

#ifndef __ASSEMBLY__

/*****************************************************************************
 * Internal helper macros used by the hwspinlock lock implementation.
 ****************************************************************************/
/* Convert a ticket to priority */
#define PRIORITY(t, pos)	(((t) << 8) | (pos))

#define CHOOSING_TICKET		0x1
#define CHOSEN_TICKET		0x0

#define hwspinlock_is_choosing(info)	(info & 0x1)
#define hwspinlock_ticket_number(info)	((info >> 1) & 0x7FFF)
#define make_hwspinlock_data(choosing, number) \
		(((choosing & 0x1) | (number << 1)) & 0xFFFF)

/*****************************************************************************
 * External hwspinlock lock interface.
 ****************************************************************************/
/*
 * Hwspin locks are stored in coherent memory
 *s
 * Each lock's data is contiguous and fully allocated by the compiler
 */

struct hwspinlock_lock {
	/*
	 * The lock_data is a bit-field of 2 members:
	 * Bit[0]       : choosing. This field is set when the CPU is
	 *                choosing its hwspinlock number.
	 * Bits[1 - 15] : number. This is the hwspinlock number allocated.
	 */
	volatile uint32_t lock_data[HWSPINLOCK_MAX_CPUS];
};

 /*
  * Now vHwspinLock_get and vHwspinLock_release should be avoid called in
  * interrupt context, or caused unknown issue
  */
void vBakspinLock_get(struct hwspinlock_lock *hwspinlock, uint32_t id);
void vBakspinLock_release(struct hwspinlock_lock *hwspinlock, uint32_t id);
void vBakspinLock_init(struct hwspinlock_lock **hwspinlock, uint32_t id,
		      void *hwspinlock_addr);
void vBakLockInit(uint32_t xAddr, uint32_t xCpus);
void __assert(const char *function, const char *file,
		uint32_t line, const char *assertion);

#define DEFINE_HWSPINLOCK(_name) struct hwspinlock_lock _name

#define DECLARE_HWSPINLOCK(_name) extern struct hwspinlock_lock _name


#endif /* __ASSEMBLY__ */
#endif /* __HWSPINLOCK_H__ */
