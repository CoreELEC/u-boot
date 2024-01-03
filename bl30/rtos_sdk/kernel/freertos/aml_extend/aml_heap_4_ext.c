/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_heap_4_ext.h"

size_t xPortGetTotalHeapSize( void )
{
	return configTOTAL_HEAP_SIZE;
}

#ifdef configRealloc
void *pvPortRealloc( void *ptr, size_t size )
{
	uint8_t *puc = ( uint8_t * ) ptr;
	BlockLink_t *pxLink;
	size_t oldlen, len;
	void *pvReturn = NULL;

	if (!ptr) {
		pvReturn = pvPortMalloc(size);
	} else {
		puc -= xHeapStructSize;
		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;
		/* Check the block is actually allocated. */
		configASSERT( ( pxLink->xBlockSize & heapBLOCK_ALLOCATED_BITMASK ) != 0 );
		configASSERT( pxLink->pxNextFreeBlock == NULL );

		oldlen = pxLink->xBlockSize & ~heapBLOCK_ALLOCATED_BITMASK;
		len = oldlen < size ? oldlen : size;
		pvReturn = pvPortMalloc(size);
		memcpy(pvReturn, ptr, len);
		free(ptr);
	}
	return pvReturn;
}
#endif
