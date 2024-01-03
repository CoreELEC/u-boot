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

size_t xPortGetTotalHeapSize(void)
{
	return xTotalHeapBytes;
}

void vPortAddHeapRegion(uint8_t *pucStartAddress, size_t xSizeInBytes)
{
	BlockLink_t *pxLink = NULL, *pxPreviousFreeBlock;
	size_t xAlignedHeap;
	size_t xTotalRegionSize;
	size_t xAddress;
	HeapRegion_t region[2];
	int bneedsus = 0;

	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
		bneedsus = 1;
	if (bneedsus)
		vTaskSuspendAll();
	if (!pxEnd)
	{
		memset(region, 0, sizeof(region));
		region[0].pucStartAddress = pucStartAddress;
		region[0].xSizeInBytes = xSizeInBytes;
		vPortDefineHeapRegions(region);
	}
	else
	{
		xAddress = (size_t)pucStartAddress;
		xTotalRegionSize = xSizeInBytes;
		if ((xAddress & portBYTE_ALIGNMENT_MASK) != 0)
		{
			xAddress += (portBYTE_ALIGNMENT - 1);
			xAddress &= ~portBYTE_ALIGNMENT_MASK;
			xTotalRegionSize -= xAddress - (size_t)pucStartAddress;
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		if (xTotalRegionSize > heapMINIMUM_BLOCK_SIZE)
		{
			xAlignedHeap = xAddress;
			pxLink = (BlockLink_t *)xAlignedHeap;

			if (pxLink <= pxEnd)
			{
				pxLink->xBlockSize = (size_t)xTotalRegionSize;
				xFreeBytesRemaining += pxLink->xBlockSize;
				xTotalHeapBytes += pxLink->xBlockSize;
				prvInsertBlockIntoFreeList(((BlockLink_t *)pxLink));
			}
			else
			{
				pxPreviousFreeBlock = pxEnd;
				xAddress = xAlignedHeap + xTotalRegionSize;
				xAddress -= xHeapStructSize;
				xAddress &= ~portBYTE_ALIGNMENT_MASK;
				pxEnd = (BlockLink_t *)xAddress;
				pxEnd->xBlockSize = 0;
				pxEnd->pxNextFreeBlock = NULL;

				pxPreviousFreeBlock->pxNextFreeBlock = pxLink;
				pxLink->xBlockSize = xAddress - (size_t)pxLink;
				pxLink->pxNextFreeBlock = pxEnd;
				xFreeBytesRemaining += pxLink->xBlockSize;
				xTotalHeapBytes += pxLink->xBlockSize;
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	if (bneedsus)
		xTaskResumeAll();
}

static char *_pxGetAlignedAddr(BlockLink_t *pxBlock, size_t xWantedSize,
							   size_t xAlignMsk, int alloc)
{
	char *p = (char *)pxBlock;
	char *end = p + pxBlock->xBlockSize;
	if (pxBlock->pxNextFreeBlock == NULL)
		return NULL;
	if (xAlignMsk < portBYTE_ALIGNMENT_MASK)
		xAlignMsk = portBYTE_ALIGNMENT_MASK;
	if (alloc)
	{
		xWantedSize -= xHeapStructSize;
		p += xHeapStructSize;
	}
	p = (char *)((((unsigned long)p) + xAlignMsk) & (~xAlignMsk));
	if ((unsigned long)p >= (unsigned long)pxBlock && (unsigned long)end > (unsigned long)p && (unsigned long)(end - p) >= xWantedSize)
	{
		return p;
	}
	else
		return NULL;
}

void *early_reserve_pages(size_t xWantedSize)
{
	int i = 0, j;
	size_t xRegionSize;
	size_t xAddress, xEndAddr, xAlignedAddress;
	configASSERT((xWantedSize & 0xFFF) == 0);
	configASSERT(pxEnd == NULL);
	for (i = 0; xDefRegion[i].xSizeInBytes; i++)
	{
		xAddress = (size_t)xDefRegion[i].pucStartAddress;
		xRegionSize = xDefRegion[i].xSizeInBytes;
		if ((xAddress & portBYTE_ALIGNMENT_MASK) != 0)
		{
			xAddress += (portBYTE_ALIGNMENT - 1);
			xAddress &= ~portBYTE_ALIGNMENT_MASK;
			xRegionSize -= xAddress - (size_t)xDefRegion[i].pucStartAddress;
			xDefRegion[i].pucStartAddress = (void *)xAddress;
			xDefRegion[i].xSizeInBytes = xRegionSize;
		}
		xEndAddr = xAddress + xRegionSize;
		if (xAddress & 0xFFF)
		{
			xAlignedAddress = ((xAddress + 0xFFF) & ~0xFFFUL);
			if ((xAlignedAddress >= xEndAddr) || (xDefRegion[MAX_REGION_CNT - 1].xSizeInBytes != 0))
				continue;
			for (j = MAX_REGION_CNT - 1; j > (i + 1); j--)
			{
				xDefRegion[j] = xDefRegion[j - 1];
			}
			xDefRegion[i].xSizeInBytes = xAlignedAddress - xAddress;
			xDefRegion[i + 1].pucStartAddress = (void *)xAlignedAddress;
			xDefRegion[i + 1].xSizeInBytes = xEndAddr - xAlignedAddress;
		}
		else if ((xAddress + xWantedSize) <= xEndAddr)
		{
			xAlignedAddress = xAddress + xWantedSize;
			xDefRegion[i].pucStartAddress = (void *)xAlignedAddress;
			if (xAlignedAddress != xEndAddr)
				xDefRegion[i].xSizeInBytes = xEndAddr - xAlignedAddress;
			else
				xDefRegion[i].xSizeInBytes = 1;
			return (void *)xAddress;
		}
	}
	return NULL;
}

void *pvPortMallocRsvAlign(size_t xWantedSize, size_t xAlignMsk)
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;
	unsigned long flags;

	if (xWantedSize <= 0)
		return pvReturn;
	configASSERT(((xAlignMsk + 1) & xAlignMsk) == 0);

#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
	portIRQ_SAVE(flags);
#else
	vTaskSuspendAll();
#endif
	{
		/* If this is the first call to malloc then the heap will require
		   initialisation to setup the list of free blocks. */
		if (pxEnd == NULL)
		{
			vPortDefineHeapRegions(NULL);
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if ((xWantedSize & heapBLOCK_ALLOCATED_BITMASK) == 0)
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if (xWantedSize > 0)
			{
				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00)
				{
					/* Byte alignment required. */
					xWantedSize += (portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK));
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining))
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while (((pvReturn = _pxGetAlignedAddr(pxBlock, xWantedSize, xAlignMsk, 0)) == NULL) && (pxBlock->pxNextFreeBlock != NULL))
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if (pvReturn)
				{
					BlockLink_t *pxTmp = (BlockLink_t *)(((uint8_t *)pvReturn));
					long tmplen = (uint8_t *)pxTmp - (uint8_t *)pxBlock;
					long tmplen2 = pxBlock->xBlockSize - tmplen;
					pxTmp->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
					pxTmp->xBlockSize = tmplen2;

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					if (tmplen >= (long)xHeapStructSize)
					{
						pxBlock->xBlockSize = tmplen;
						pxPreviousBlock = pxBlock;
					}
					else
					{
						pxPreviousBlock->pxNextFreeBlock = pxTmp->pxNextFreeBlock;
					}
					pxBlock = pxTmp;

					/* If the block is larger than required it can be split into
					two. */
					if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE)
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList((pxNewBlockLink));
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining)
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= heapBLOCK_ALLOCATED_BITMASK;
					pxBlock->pxNextFreeBlock = NULL;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC(pvReturn, xWantedSize);
	}

#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
	portIRQ_RESTORE(flags);
#else
	(void)xTaskResumeAll();
#endif

	return pvReturn;
}

void *pvPortMallocAlign(size_t xWantedSize, size_t xAlignMsk)
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;
	unsigned long flags;

#ifdef CONFIG_MEMORY_ERROR_DETECTION
	size_t dMallocsz = xWantedSize;
#endif

	if (xWantedSize <= 0)
		return pvReturn;

	configASSERT(((xAlignMsk + 1) & xAlignMsk) == 0);

#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
	portIRQ_SAVE(flags);
#else
	vTaskSuspendAll();
#endif
	{
		/* If this is the first call to malloc then the heap will require
		   initialisation to setup the list of free blocks. */
		if (pxEnd == NULL)
		{
			vPortDefineHeapRegions(NULL);
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if ((xWantedSize & heapBLOCK_ALLOCATED_BITMASK) == 0)
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if (xWantedSize > 0)
			{
				xWantedSize += xHeapStructSize;

#ifdef CONFIG_MEMORY_ERROR_DETECTION
				xWantedSize += sizeof(size_t); // add size of tail_canary
#endif

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00)
				{
					/* Byte alignment required. */
					xWantedSize += (portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK));
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining))
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;

				while (((pvReturn = _pxGetAlignedAddr(pxBlock, xWantedSize, xAlignMsk, 1)) == NULL) && (pxBlock->pxNextFreeBlock != NULL))
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if (pvReturn)
				{
					BlockLink_t *pxTmp = (BlockLink_t *)(((uint8_t *)pvReturn) - xHeapStructSize);

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					if ((unsigned long)pxTmp > (unsigned long)pxBlock)
					{
						long tmplen = (uint8_t *)pxTmp - (uint8_t *)pxBlock;
						configASSERT(tmplen >= (long)xHeapStructSize);
						pxTmp->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
						pxTmp->xBlockSize = pxBlock->xBlockSize - tmplen;
						pxBlock->xBlockSize = tmplen;
						pxPreviousBlock = pxBlock;
						pxBlock = pxTmp;
					}
					else
					{
						configASSERT(pxTmp == pxBlock);
						pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
					}

					/* If the block is larger than required it can be split into
					two. */
					if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE)
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList((pxNewBlockLink));
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining)
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= heapBLOCK_ALLOCATED_BITMASK;
					pxBlock->pxNextFreeBlock = NULL;

#ifdef CONFIG_MEMORY_ERROR_DETECTION
					/* memory request record */
					vPortAddToList((size_t)(((uint8_t *)pvReturn) - xHeapStructSize), dMallocsz);
#endif

#ifdef CONFIG_DMALLOC
					/* memory request record by dmalloc */
					vdRecordMalloc(xWantedSize);
#endif
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC(pvReturn, xWantedSize);
	}
#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
	portIRQ_RESTORE(flags);
#else
	(void)xTaskResumeAll();
#endif

#if (configUSE_MALLOC_FAILED_HOOK == 1)
	{
		if (pvReturn == NULL)
		{
			extern void vApplicationMallocFailedHook(void);
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
#endif

	return pvReturn;
}
/*-----------------------------------------------------------*/

