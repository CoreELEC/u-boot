/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __HWSPINLOCK_H__
#define __HWSPINLOCK_H__

#include "hwspinlock_config.h"

#define AML_MUTEX_AP_PROC_ID 1
#define AML_MUTEX_AO_PROC_ID 2

struct hwspinlock_t {
	volatile uint32_t lock_data[HWSPINLOCK_MAX_CPUS];
};



 /*
  * Now vHwspinLock_get and vHwspinLock_release should be avoid called in
  * interrupt context, or caused unknown issue
  */
BaseType_t vHwspinLock_get(struct hwspinlock_t *hwspinlock, uint32_t id);
void vHwspinLock_release(struct hwspinlock_t *hwspinlock, uint32_t id);
void vHwspinLock_init(struct hwspinlock_t **hwspinlock, uint32_t id,
		      void *hwspinlock_addr);
void vHwLockInit(uint32_t xAddr, uint32_t xCpus);

#define DEFINE_HWSPINLOCK(_name) struct hwspinlock_t _name

#define DECLARE_HWSPINLOCK(_name) extern struct hwspinlock_t _name

#endif /* __HWSPINLOCK_H__ */
