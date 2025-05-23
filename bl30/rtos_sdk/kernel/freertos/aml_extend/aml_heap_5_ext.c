/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_heap_5_ext.h"
#include <stdio.h>

int vPrintFreeListAfterMallocFail(void)
{
	BlockLink_t *pxIterator;
	int total_free_size = 0;

	for (pxIterator = &xStart; pxIterator != pxEnd; pxIterator = pxIterator->pxNextFreeBlock)
	{
		printf("the address: %p, len: %d\n", pxIterator, (int)(pxIterator->xBlockSize));
		total_free_size += (pxIterator->xBlockSize);
	}
	printf("the total free size: %d\n", total_free_size);

	return 0;
}

void *xPortRealloc(void *ptr, size_t size)
{
	void *p = NULL;
	size_t old_len = 0;
	size_t len = 0;

	if (ptr)
	{
		BlockLink_t *pxTmp = (BlockLink_t *)(((uint8_t *)ptr) - xHeapStructSize);
		old_len = pxTmp->xBlockSize - xHeapStructSize;
		len = old_len < size ? old_len : size;
		if (!size)
		{
			vPortFree(ptr);
			return NULL;
		}

		p = pvPortMalloc(size);
		if (p)
		{
			memcpy(p, ptr, len);
			if (size > len)
				memset((char *)p + len, 0, size - len);
		}
		vPortFree(ptr);
	}
	else
	{
		p = pvPortMalloc(size);
	}

	return p;
}
