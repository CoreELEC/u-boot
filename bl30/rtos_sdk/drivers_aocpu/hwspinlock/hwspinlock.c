/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "hwspinlock.h"
#include "uart.h"
#include "register.h"

#ifndef PRINT_DBG
#define PRINT_DBG(...)          //printf(__VA_ARGS__)
#define PRINT_ERR(...)          printf(__VA_ARGS__)
#define PRINT(...)              printf(__VA_ARGS__)
#endif
#define HWTAG	"HWSPIN"

SemaphoreHandle_t xHwLockSemp[HWSPINLOCK_NUMS];
struct hwspinlock_prop {
	uint32_t xAddr;
	uint32_t xCpus;
};
struct hwspinlock_prop prop;

BaseType_t vHwspinLock_get(struct hwspinlock_t *hwspinlock, uint32_t id)
{
	BaseType_t ret = pdTRUE;
	uint32_t ulLockVal = hwspinlock->lock_data[0];
	uint32_t ulCnt = 0;

	if (ulLockVal != 0) {
		PRINT_ERR("[%s]: locked0 idx %x by %x\n", HWTAG, id, ulLockVal);
		return pdFALSE;
	}
	ret = xSemaphoreTake(xHwLockSemp[id], portMAX_DELAY);
	if (ret == pdFALSE) {
		PRINT_ERR("[%s]: Async get sempaphore fail\n", HWTAG);
		configASSERT(ret);
	}
lock_retry:
	hwspinlock->lock_data[0] = AML_MUTEX_AO_PROC_ID;
	ulLockVal = hwspinlock->lock_data[0];
	if (ulLockVal == 0 && ulCnt < 3) {
		ulCnt += 1;
		goto lock_retry;
	}
	if (AML_MUTEX_AO_PROC_ID != ulLockVal) {
		PRINT_ERR("[%s]: locked1 idx %x by %x\n", HWTAG, id, ulLockVal);
		xSemaphoreGive(xHwLockSemp[id]);
		ret = pdFALSE;
	}
	return ret;
}

void vHwspinLock_release(struct hwspinlock_t *hwspinlock, uint32_t id)
{
	uint32_t ulLockVal = 0;

	if (uxSemaphoreGetCount(xHwLockSemp[id])) {
		PRINT_ERR("[%s]: No hwspinlock required before\n", HWTAG);
		return;
	}

	ulLockVal = hwspinlock->lock_data[0];
	if (ulLockVal != AML_MUTEX_AO_PROC_ID) {
		PRINT_ERR("[%s]: Release idx %x locked by\n", HWTAG, id, ulLockVal);
		return;
	}

	hwspinlock->lock_data[0] = AML_MUTEX_AO_PROC_ID;
	xSemaphoreGive(xHwLockSemp[id]);
}

void vHwspinLock_init(struct hwspinlock_t **hwspinlock, uint32_t id,
		      void *hwspinlock_addr)
{
	uint32_t xAddrOff = id * sizeof(struct hwspinlock_t);

	if (!hwspinlock_addr)
		hwspinlock_addr = (char *)prop.xAddr + xAddrOff;
	*hwspinlock = hwspinlock_addr;
	PRINT_DBG("hwspinlock addr %p\n", hwspinlock_addr);
}

void vHwLockInit(uint32_t xAddr, uint32_t xCpus)
{
	int idx = 0;

	prop.xAddr = xAddr;
	prop.xCpus = xCpus;

	for (idx = 0; idx < HWSPINLOCK_NUMS; idx++) {
		xHwLockSemp[idx] = xSemaphoreCreateBinary();
		configASSERT(xHwLockSemp[idx]);
		xSemaphoreGive(xHwLockSemp[idx]);
	}
	PRINT_ERR("hwspinlock init\n");
}
