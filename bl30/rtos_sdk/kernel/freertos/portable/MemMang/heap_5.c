/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * A sample implementation of pvPortMalloc() that allows the heap to be defined
 * across multiple non-contigous blocks and combines (coalescences) adjacent
 * memory blocks as they are freed.
 *
 * See heap_1.c, heap_2.c, heap_3.c and heap_4.c for alternative
 * implementations, and the memory management pages of http://www.FreeRTOS.org
 * for more information.
 *
 * Usage notes:
 *
 * vPortDefineHeapRegions() ***must*** be called before pvPortMalloc().
 * pvPortMalloc() will be called if any task objects (tasks, queues, event
 * groups, etc.) are created, therefore vPortDefineHeapRegions() ***must*** be
 * called before any other objects are defined.
 *
 * vPortDefineHeapRegions() takes a single parameter.  The parameter is an array
 * of HeapRegion_t structures.  HeapRegion_t is defined in portable.h as
 *
 * typedef struct HeapRegion
 * {
 *	uint8_t *pucStartAddress; << Start address of a block of memory that will be part of the heap.
 *	size_t xSizeInBytes;	  << Size of the block of memory.
 * } HeapRegion_t;
 *
 * The array is terminated using a NULL zero sized region definition, and the
 * memory regions defined in the array ***must*** appear in address order from
 * low address to high address.  So the following is a valid example of how
 * to use the function.
 *
 * HeapRegion_t xHeapRegions[] =
 * {
 * 	{ ( uint8_t * ) 0x80000000UL, 0x10000 }, << Defines a block of 0x10000 bytes starting at address 0x80000000
 * 	{ ( uint8_t * ) 0x90000000UL, 0xa0000 }, << Defines a block of 0xa0000 bytes starting at address of 0x90000000
 * 	{ NULL, 0 }                << Terminates the array.
 * };
 *
 * vPortDefineHeapRegions( xHeapRegions ); << Pass the array into vPortDefineHeapRegions().
 *
 * Note 0x80000000 is the lower address so appears in the array first.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if (configSUPPORT_DYNAMIC_ALLOCATION == 0)
#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE ((size_t)8)

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
#ifdef CONFIG_MEMORY_ERROR_DETECTION
	size_t head_canary; /*<< Head Canary, TODO: Remove */
#endif
	struct A_BLOCK_LINK *pxNextFreeBlock; /*<< The next free block in the list. */
	size_t xBlockSize;					  /*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert);

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize = (sizeof(BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) & ~((size_t)portBYTE_ALIGNMENT_MASK);

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;
static size_t xTotalHeapBytes = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/* Allocate the memory for the heap. */
#if CONFIG_RISCV
#define MAX_REGION_CNT 2
static HeapRegion_t xDefRegion[MAX_REGION_CNT + 1] =
	{
		{(uint8_t *)configDEFAULT_HEAP_ADDR, configDEFAULT_HEAP_SIZE},
		{0, 0},
		{0, 0}};
#else
#define MAX_REGION_CNT 2
extern uint8_t _heap_start[];
extern uint8_t _heap_len[];
static HeapRegion_t xDefRegion[MAX_REGION_CNT + 1] =
	{
		{_heap_start, (size_t)_heap_len},
		{0, 0},
		{0, 0}};
#endif

/*-----------------------------------------------------------*/

#ifdef CONFIG_MEMORY_ERROR_DETECTION
#include "aml_med_ext.c"
#endif

#ifdef CONFIG_DMALLOC
#include "aml_dmalloc_ext.c"
#endif

void *pvPortMalloc(size_t xWantedSize)
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;
	unsigned long flags;

#ifdef CONFIG_MEMORY_ERROR_DETECTION
	size_t dMallocsz = xWantedSize;
#endif

	if (xWantedSize <= 0)
		return pvReturn;

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
		if ((xWantedSize & xBlockAllocatedBit) == 0)
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

				while ((pxBlock->xBlockSize < xWantedSize) && (pxBlock->pxNextFreeBlock != NULL))
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if (pxBlock != pxEnd)
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextFreeBlock) + xHeapStructSize);

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

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
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;

#ifdef CONFIG_MEMORY_ERROR_DETECTION
					/* memory request record by med */
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
		if ((xWantedSize & xBlockAllocatedBit) == 0)
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
					pxBlock->xBlockSize |= xBlockAllocatedBit;
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
		if ((xWantedSize & xBlockAllocatedBit) == 0)
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
					pxBlock->xBlockSize |= xBlockAllocatedBit;
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

void vPortFree(void *pv)
{
	uint8_t *puc = (uint8_t *)pv;
	BlockLink_t *pxLink;
	unsigned long flags;

	if (pv != NULL)
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = (void *)puc;

		/* Check the block is actually allocated. */
		configASSERT((pxLink->xBlockSize & xBlockAllocatedBit) != 0);
		configASSERT(pxLink->pxNextFreeBlock == NULL);

#ifdef CONFIG_DMALLOC
		/* memory request record by dmalloc */
		vdRecordFree(pxLink->xBlockSize);
#endif

		if ((pxLink->xBlockSize & xBlockAllocatedBit) != 0)
		{
			if (pxLink->pxNextFreeBlock == NULL)
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
				portIRQ_SAVE(flags);
#else
				vTaskSuspendAll();
#endif
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					traceFREE(pv, pxLink->xBlockSize);
#ifdef CONFIG_MEMORY_ERROR_DETECTION
					vPortRmFromList((size_t)pxLink);
#endif
					prvInsertBlockIntoFreeList(((BlockLink_t *)pxLink));
#ifdef CONFIG_MEMORY_ERROR_DETECTION
					vPortUpdateFreeBlockList();
#endif
				}
#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
				portIRQ_RESTORE(flags);
#else
				(void)xTaskResumeAll();
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
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize(void)
{
	return xFreeBytesRemaining;
}
size_t xPortGetTotalHeapSize(void)
{
	return xTotalHeapBytes;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize(void)
{
	return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert)
{
	BlockLink_t *pxIterator;
	uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for (pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock)
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = (uint8_t *)pxIterator;

	if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert)
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = (uint8_t *)pxBlockToInsert;

	if ((puc + pxBlockToInsert->xBlockSize) == (uint8_t *)pxIterator->pxNextFreeBlock)
	{
		if (pxIterator->pxNextFreeBlock != pxEnd)
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if (pxIterator != pxBlockToInsert)
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}
/*-----------------------------------------------------------*/

void vPortDefineHeapRegions(const HeapRegion_t *const pRegions)
{
	BlockLink_t *pxFirstFreeBlockInRegion = NULL, *pxPreviousFreeBlock;
	size_t xAlignedHeap;
	size_t xTotalRegionSize, xTotalHeapSize = 0;
	BaseType_t xDefinedRegions = 0;
	size_t xAddress;
	const HeapRegion_t *pxHeapRegion;

	const HeapRegion_t *pxHeapRegions = pRegions;

	/* Can only call once! */
	configASSERT(pxEnd == NULL);

	if (!pxHeapRegions)
		pxHeapRegions = xDefRegion;

	pxHeapRegion = &(pxHeapRegions[xDefinedRegions]);

	while (pxHeapRegion->xSizeInBytes > 0)
	{
		xTotalRegionSize = pxHeapRegion->xSizeInBytes;

		/* Ensure the heap region starts on a correctly aligned boundary. */
		xAddress = (size_t)pxHeapRegion->pucStartAddress;
		if ((xAddress & portBYTE_ALIGNMENT_MASK) != 0)
		{
			xAddress += (portBYTE_ALIGNMENT - 1);
			xAddress &= ~portBYTE_ALIGNMENT_MASK;

			/* Adjust the size for the bytes lost to alignment. */
			xTotalRegionSize -= xAddress - (size_t)pxHeapRegion->pucStartAddress;
		}
		if (xTotalRegionSize < 2 * xHeapStructSize)
		{
			xDefinedRegions++;
			pxHeapRegion = &(pxHeapRegions[xDefinedRegions]);
			continue;
		}

		xAlignedHeap = xAddress;

		/* Set xStart if it has not already been set. */
		if (xDefinedRegions == 0)
		{
			/* xStart is used to hold a pointer to the first item in the list of
			free blocks.  The void cast is used to prevent compiler warnings. */
			xStart.pxNextFreeBlock = (BlockLink_t *)xAlignedHeap;
			xStart.xBlockSize = (size_t)0;
		}
		else
		{
			/* Should only get here if one region has already been added to the
			heap. */
			configASSERT(pxEnd != NULL);

			/* Check blocks are passed in with increasing start addresses. */
			configASSERT(xAddress > (size_t)pxEnd);
		}

		/* Remember the location of the end marker in the previous region, if
		any. */
		pxPreviousFreeBlock = pxEnd;

		/* pxEnd is used to mark the end of the list of free blocks and is
		inserted at the end of the region space. */
		xAddress = xAlignedHeap + xTotalRegionSize;
		xAddress -= xHeapStructSize;
		xAddress &= ~portBYTE_ALIGNMENT_MASK;
		pxEnd = (BlockLink_t *)xAddress;
		pxEnd->xBlockSize = 0;
		pxEnd->pxNextFreeBlock = NULL;

		/* To start with there is a single free block in this region that is
		sized to take up the entire heap region minus the space taken by the
		free block structure. */
		pxFirstFreeBlockInRegion = (BlockLink_t *)xAlignedHeap;
		pxFirstFreeBlockInRegion->xBlockSize = xAddress - (size_t)pxFirstFreeBlockInRegion;
		pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd;

		/* If this is not the first region that makes up the entire heap space
		then link the previous region to this region. */
		if (pxPreviousFreeBlock != NULL)
		{
			pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
		}

		xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

		/* Move onto the next HeapRegion_t structure. */
		xDefinedRegions++;
		pxHeapRegion = &(pxHeapRegions[xDefinedRegions]);
	}

	xMinimumEverFreeBytesRemaining = xTotalHeapSize;
	xFreeBytesRemaining = xTotalHeapSize;
	xTotalHeapBytes = xTotalHeapSize;

	/* Check something was actually defined before it is accessed. */
	configASSERT(xTotalHeapSize);

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ((size_t)1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1);
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

/* Add include implement source code which depend on the inner elements */
#include "aml_heap_5_ext.c"
