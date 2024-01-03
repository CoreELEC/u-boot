/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_HEAP_5_EXT_H__
#define __AML_HEAP_5_EXT_H__

#include <stddef.h>
#include <stdint.h>

int vPrintFreeListAfterMallocFail(void);

void *xPortRealloc(void *ptr, size_t size);

size_t xPortGetTotalHeapSize(void);

void vPortAddHeapRegion(uint8_t *pucStartAddress, size_t xSizeInBytes);

void *early_reserve_pages(size_t xWantedSize);

void *pvPortMallocRsvAlign(size_t xWantedSize, size_t xAlignMsk);

void *pvPortMallocAlign(size_t xWantedSize, size_t xAlignMsk);

#endif
