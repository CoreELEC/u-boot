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
#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
#include <printk.h>
#include "sys_printf.h"
#if CONFIG_BACKTRACE
#include "stack_trace.h"
#endif
#endif

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
#ifdef CONFIG_DMALLOC
struct MemLeak MemLeak_t[CONFIG_DMALLOC_SIZE];
#endif

#ifdef CONFIG_MEMORY_ERROR_DETECTION

#define UNWIND_DEPTH 5
#define RAM_REGION_NUMS 2
#define HEAD_CANARY_PATTERN (size_t)0x5051525354555657
#define TAIL_CANARY_PATTERN (size_t)0x6061626364656667
#define HEAD_CANARY(x) ((x)->head_canary)
#define TAIL_CANARY(x, y) *(size_t *)((size_t)(x) + ((y) & ~xBlockAllocatedBit) - sizeof(size_t))

/* Used to define the bss and data segments in the program. */
typedef struct
{
	size_t *startAddress;
	size_t size;
} tMemoryRegion;
/* Assignment Process Tracker */
typedef struct
{
	BlockLink_t *allocHandle;
	TaskHandle_t xOwner;
	size_t blockSize;
	size_t requestSize;
	unsigned long backTrace[UNWIND_DEPTH];
} allocTraceBlock_t;

/* allocation buffer pool tracking alloc */
allocTraceBlock_t allocList[CONFIG_MEMORY_ERROR_DETECTION_SIZE] = {NULL};

#if CONFIG_N200_REVA
// NOTHING
#else
extern uint8_t _bss_start[];
extern uint8_t _bss_len[];
extern uint8_t _data_start[];
extern uint8_t _data_len[];
tMemoryRegion globalRam[RAM_REGION_NUMS] = {
	{(size_t *)_bss_start, (size_t)_bss_len},
	{(size_t *)_data_start, (size_t)_data_len}};
#endif

/* For pinpointing the location of anomalies(dump stack) */
// print_traceitem
static void print_traceitem(unsigned long *trace)
{
#if CONFIG_BACKTRACE
	printk("\tCallTrace:\n");
	for (int i = 0; i < UNWIND_DEPTH; i++)
	{
		printk("\t");
		print_symbol(*(trace + i));
	}
#endif
	return;
}
// get_calltrace
static void get_calltrace(unsigned long *trace)
{
#if CONFIG_BACKTRACE
#define CT_SKIP 2
	int32_t ret, i;
	unsigned long _trace[32];
	ret = get_backtrace(NULL, _trace, UNWIND_DEPTH + CT_SKIP);
	if (ret)
	{
		for (i = 0; i < UNWIND_DEPTH; i++)
		{
			trace[i] = _trace[i + CT_SKIP];
		}
	}
#endif
}
// On-site printing from memory
#ifdef CONFIG_MEMORY_ERROR_DETECTION_PRINT
static void print_memory_site_info(uint8_t *address)
{
#define STEP_VALUE_FOR_MEMORY 8

	int step;

	printk("Memory Request Address Field Details (ADDRESS:0x%08x)\n", (size_t)address);
	printk("\t ADDRESS -%d\n", STEP_VALUE_FOR_MEMORY * sizeof(size_t));

	for (step = (STEP_VALUE_FOR_MEMORY - 1); step >= 0; step--){
		printk("\t");
		for (int loops = sizeof(size_t); loops > 0; loops--){
			printk("%02x ", *(address - loops - step * sizeof(size_t)));
		}
		printk("\n");
	}

	printk("\t ADDRESS\n");

	for (step = 0; step < STEP_VALUE_FOR_MEMORY; step++){
		printk("\t");
		for (int loops = 0; loops < sizeof(size_t); loops++){
			printk("%02x ", *(address + loops + step * sizeof(size_t)));
		}
		printk("\n");
	}
}
#endif
/* Additional functions to scan memory (buffer overflow, memory leaks) */
// vPortUpdateFreeBlockList
static void vPortUpdateFreeBlockList(void)
{
	BlockLink_t *start = &xStart;
	do
	{
		HEAD_CANARY(start) = HEAD_CANARY_PATTERN;
		start = start->pxNextFreeBlock;
	} while (start->pxNextFreeBlock != NULL);
}
// vPortAddToList
static void vPortAddToList(size_t pointer, size_t tureSize)
{
	size_t pos = 0;
	/* Set up an out-of-bounds protected area */
	BlockLink_t *temp = (BlockLink_t *)pointer;
	HEAD_CANARY(temp) = HEAD_CANARY_PATTERN;
	TAIL_CANARY(temp, temp->xBlockSize) = TAIL_CANARY_PATTERN;
	/* Fill Tracking Info Block */
	while (pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE)
	{
		if (!allocList[pos].allocHandle)
		{
			allocList[pos].requestSize = tureSize;
			/* cache block size */
			allocList[pos].blockSize = temp->xBlockSize;
			/* mount malloc point */
			allocList[pos].allocHandle = (BlockLink_t *)pointer;
			/* get call stack info */
			get_calltrace(allocList[pos].backTrace);
			/* Current task owener */
			if (xTaskGetCurrentTaskHandle() && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
			{
				allocList[pos].xOwner = xTaskGetCurrentTaskHandle();
			}
			else
			{
				allocList[pos].xOwner = NULL;
			}
			break;
		}
		/* increment */
		pos++;
	}
	/* update free block list */
	vPortUpdateFreeBlockList();
}
// vPortRmFromList
static void vPortRmFromList(size_t pointer)
{
	size_t pos;
	/* The allocated address of the current block */
	size_t allocatedAddress = xHeapStructSize + pointer;
	/* Check if the task is freed */
	for (pos = 0; pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE; pos++)
	{
		if (((size_t)(allocList[pos].xOwner)) == allocatedAddress)
		{
			allocList[pos].xOwner = NULL;
		}
	}
	/* Release the specified tracking block */
	for (pos = 0; pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE; pos++)
	{
		if (allocList[pos].allocHandle == (BlockLink_t *)pointer)
		{
			allocList[pos].xOwner = NULL;
			allocList[pos].blockSize = 0;
			allocList[pos].requestSize = 0;
			allocList[pos].allocHandle = NULL;
			memset(allocList[pos].backTrace, 0, sizeof(allocList[pos].backTrace));
			break;
		}
	}
}
// Check memory node for overflow?
int xCheckMallocNodeIsOver(void *node)
{
	unsigned long flags;
	size_t pos = 0, ret = 0;
	size_t buffer_address, buffer_size;
	BlockLink_t *allocHandle = NULL;

#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
	portIRQ_SAVE(flags);
#else
	vTaskSuspendAll();
#endif

	/* get node handle */
	allocHandle = (BlockLink_t *)((size_t)(node)-xHeapStructSize);

	/* Find node buffer pool */
	while (pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE) {
		if (allocList[pos].allocHandle == allocHandle)
			break;
		pos++;
	}

	/* Header integrity check */
	if (HEAD_CANARY(allocHandle) != HEAD_CANARY_PATTERN) {
		printk("ERROR!!! detected buffer overflow(HEAD)\r\n");
		buffer_address = (size_t)node;
		buffer_size = allocHandle->xBlockSize & ~xBlockAllocatedBit;
		/* Get monitoring nodes */
		if (pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE) {
			if (allocList[pos].xOwner) {
				TaskStatus_t status;

				vTaskGetInfo(allocList[pos].xOwner, &status, 0, 0);
				printk(
					"\tTask owner:(%s) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
				    status.pcTaskName, buffer_address, allocList[pos].requestSize,
					buffer_size);
			} else {
				printk(
				    "\tTask owner:(NULL) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
				    buffer_address, allocList[pos].requestSize, buffer_size);
			}
			print_traceitem(allocList[pos].backTrace);
		}
#ifdef CONFIG_MEMORY_ERROR_DETECTION_PRINT
		print_memory_site_info((uint8_t *)buffer_address);
#endif
		ret = 1;
	}
	/* Tail integrity check */
	if (TAIL_CANARY(allocHandle, allocHandle->xBlockSize) != TAIL_CANARY_PATTERN) {
		printk("ERROR!!! detected buffer overflow(TAIL)\r\n");
		buffer_address = (size_t)node;
		buffer_size = allocHandle->xBlockSize & ~xBlockAllocatedBit;
		/* Get monitoring nodes */
		if (pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE) {
			if (allocList[pos].xOwner) {
				TaskStatus_t status;

				vTaskGetInfo(allocList[pos].xOwner, &status, 0, 0);
				printk(
				    "\tTask owner:(%s) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
				    status.pcTaskName, buffer_address, allocList[pos].requestSize,
					buffer_size);
			} else {
				printk(
				    "\tTask owner:(NULL) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
				    buffer_address, allocList[pos].requestSize, buffer_size);
			}
			print_traceitem(allocList[pos].backTrace);
		}
#ifdef CONFIG_MEMORY_ERROR_DETECTION_PRINT
		print_memory_site_info((uint8_t *)buffer_address);
#endif
		ret = 1;
	}

#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
	portIRQ_RESTORE(flags);
#else
	(void)xTaskResumeAll();
#endif

	return ret;
}
// Check for out of bounds memory
int xPortCheckIntegrity(void)
{
	int result = 0;
	size_t pos = 0;

	/* Scan free memory list integrity */
	BlockLink_t *start = &xStart;
	do
	{
		configASSERT(HEAD_CANARY(start) == HEAD_CANARY_PATTERN);
		start = start->pxNextFreeBlock;
	} while (start->pxNextFreeBlock != NULL);

	/* Scan allocated memory integrity */
	while (pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE)
	{
		if (allocList[pos].allocHandle)
		{
			/* Header integrity check */
			if (HEAD_CANARY(allocList[pos].allocHandle) != HEAD_CANARY_PATTERN)
			{
				printk("ERROR!!! detected buffer overflow(HEAD)\r\n");

				size_t buffer_address = (size_t)(allocList[pos].allocHandle) + xHeapStructSize;
				size_t buffer_size = allocList[pos].allocHandle->xBlockSize & ~xBlockAllocatedBit;

				if (allocList[pos].xOwner)
				{
					TaskStatus_t status;
					vTaskGetInfo(allocList[pos].xOwner, &status, 0, 0);
					printk("\tTask owner:(%s) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
						   status.pcTaskName, buffer_address, allocList[pos].requestSize, buffer_size);
				}
				else
				{
					printk("\tTask owner:(NULL) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
						   buffer_address, allocList[pos].requestSize, buffer_size);
				}

				print_traceitem(allocList[pos].backTrace);
#ifdef CONFIG_MEMORY_ERROR_DETECTION_PRINT
				print_memory_site_info((uint8_t *)buffer_address);
#endif
				result++;
			}
			/* Tail integrity check */
			if (TAIL_CANARY(allocList[pos].allocHandle, allocList[pos].blockSize) != TAIL_CANARY_PATTERN)
			{
				printk("ERROR!!! detected buffer overflow(TAIL)\r\n");

				size_t buffer_address = (size_t)(allocList[pos].allocHandle) + xHeapStructSize;
				size_t buffer_size = allocList[pos].allocHandle->xBlockSize & ~xBlockAllocatedBit;

				if (allocList[pos].xOwner)
				{
					TaskStatus_t status;
					vTaskGetInfo(allocList[pos].xOwner, &status, 0, 0);
					printk("\tTask owner:(%s) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
						   status.pcTaskName, buffer_address, allocList[pos].requestSize, buffer_size);
				}
				else
				{
					printk("\tTask owner:(NULL) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
						   buffer_address, allocList[pos].requestSize, buffer_size);
				}

				print_traceitem(allocList[pos].backTrace);
#ifdef CONFIG_MEMORY_ERROR_DETECTION_PRINT
				print_memory_site_info((uint8_t *)buffer_address);
#endif
				result++;
			}
		}
		/* Judging whether the inspection is over */
		pos++;
	}

	return result;
}
// Check for orphaned memory(memleak detection)
int xPortMemoryScan(void)
{
	int result = 0;
	size_t pos = 0, idx = 0;
	size_t found, allocatedAddress;
	size_t *tempEndAddress;
	size_t *tempStartAddress;
	size_t *jumpStartAddress = (size_t *)(&allocList[0]);
	size_t *jumpEndAddress = (size_t *)((size_t)jumpStartAddress + sizeof(allocList));

	/* memory leak scan */
	while (pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE)
	{
		idx = 0;
		found = 0;
		/* Scan the specified memory mount point */
		if (allocList[pos].allocHandle)
		{
			/* Set target address */
			allocatedAddress = xHeapStructSize + (size_t)(allocList[pos].allocHandle);
			/* Scan all dynamic memory -1 */
			while (idx < CONFIG_MEMORY_ERROR_DETECTION_SIZE)
			{
				if ((pos != idx) && (allocList[idx].allocHandle))
				{
					/* Calculate the current thread stack address range */
					tempStartAddress = (size_t *)(xHeapStructSize + (size_t)(allocList[idx].allocHandle));
					tempEndAddress = (size_t *)(allocList[idx].requestSize + (size_t)tempStartAddress);
					/* scan other memory ( include thread stack & list etc ) */
					while (tempStartAddress < tempEndAddress)
					{
						if (*tempStartAddress == allocatedAddress)
						{
							found = 1;
							break;
						}
						/* Increase address by size_t */
						tempStartAddress++;
					}
				}
				idx++;
			}
			/* Scan all static memory area -2 */
#ifndef CONFIG_N200_REVA
			if (!found)
			{
				for (int i = 0; i < RAM_REGION_NUMS; i++)
				{
					/* Calculate the address range of the static memory area */
					tempStartAddress = globalRam[i].startAddress;
					tempEndAddress = (size_t *)((size_t)(tempStartAddress) + globalRam[i].size);
					/* scan bss & data segment */
					while (tempStartAddress < tempEndAddress)
					{
						if ((*tempStartAddress == allocatedAddress) &&
							((tempStartAddress < jumpStartAddress) || (tempStartAddress > jumpEndAddress)))
						{
							found = 1;
							break;
						}
						tempStartAddress++;
					}
				}
			}
#endif
			/* Didn't found any references to a pointer */
			if (!found)
			{
				printk("WARNING!!! detected buffer leak\r\n");

				size_t buffer_size = allocList[pos].allocHandle->xBlockSize & ~xBlockAllocatedBit;

				if (allocList[pos].xOwner)
				{
					TaskStatus_t status;
					vTaskGetInfo(allocList[pos].xOwner, &status, 0, 0);
					printk("\tTask owner:(%s) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
						   status.pcTaskName, allocatedAddress, allocList[pos].requestSize, buffer_size);
					print_traceitem(allocList[pos].backTrace);
				}
				else
				{
					printk("\tTask owner:(NULL) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
						   allocatedAddress, allocList[pos].requestSize, buffer_size);
					print_traceitem(allocList[pos].backTrace);
				}

				result++;
			}
		}
		/* next */
		pos++;
	}

	return result;
}
#endif

void *pvPortMalloc(size_t xWantedSize)
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;
	unsigned long flags;

#ifdef CONFIG_MEMORY_ERROR_DETECTION
	size_t dMallocsz = xWantedSize;
#endif

#ifdef CONFIG_DMALLOC
	int len = 0;
	int MemTaskNum = 0;
	char *taskname = pcTaskGetName(NULL);
	if (taskname != NULL)
		MemTaskNum = uxTaskGetTaskNumber(xTaskGetHandle(taskname));
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
					/* memory request record */
					vPortAddToList((size_t)(((uint8_t *)pvReturn) - xHeapStructSize), dMallocsz);
#endif

#ifdef CONFIG_DMALLOC
					if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
					{
						MemLeak_t[0].Flag = 1;
						MemLeak_t[0].TaskNum = 0;
						MemLeak_t[0].WantSize = xWantedSize;
						MemLeak_t[0].WantTotalSize += xWantedSize;
						MemLeak_t[0].MallocCount++;
						strncpy(MemLeak_t[0].TaskName, "not_in_task", 20);
						MemLeak_t[0].TaskName[sizeof(MemLeak_t[0].TaskName) - 1] = '\0';
					}
					else
					{
						if (taskname != NULL)
						{
							MemLeak_t[MemTaskNum].TaskNum = MemTaskNum;
							MemLeak_t[MemTaskNum].WantSize = xWantedSize;
							MemLeak_t[MemTaskNum].WantTotalSize += xWantedSize;
							MemLeak_t[MemTaskNum].MallocCount++;
							len = sizeof(MemLeak_t[MemTaskNum].TaskName) > strlen(taskname) ? strlen(taskname) : sizeof(MemLeak_t[MemTaskNum].TaskName);
							strncpy(MemLeak_t[MemTaskNum].TaskName, taskname, len);
							MemLeak_t[MemTaskNum].TaskName[sizeof(MemLeak_t[MemTaskNum].TaskName) - 1] = '\0';
						}
					}
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

#ifdef CONFIG_DMALLOC
	int len = 0;
	int MemTaskNum = 0;
	char *taskname = pcTaskGetName(NULL);
	if (taskname != NULL)
		MemTaskNum = uxTaskGetTaskNumber(xTaskGetHandle(taskname));
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
					if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
					{
						MemLeak_t[0].Flag = 1;
						MemLeak_t[0].TaskNum = 0;
						MemLeak_t[0].WantSize = xWantedSize;
						MemLeak_t[0].WantTotalSize += xWantedSize;
						MemLeak_t[0].MallocCount++;
						strncpy(MemLeak_t[0].TaskName, "not_in_task", 20);
						MemLeak_t[0].TaskName[sizeof(MemLeak_t[0].TaskName) - 1] = '\0';
					}
					else
					{
						if (taskname != NULL)
						{
							MemLeak_t[MemTaskNum].TaskNum = MemTaskNum;
							MemLeak_t[MemTaskNum].WantSize = xWantedSize;
							MemLeak_t[MemTaskNum].WantTotalSize += xWantedSize;
							MemLeak_t[MemTaskNum].MallocCount++;
							len = sizeof(MemLeak_t[MemTaskNum].TaskName) > strlen(taskname) ? strlen(taskname) : sizeof(MemLeak_t[MemTaskNum].TaskName);
							strncpy(MemLeak_t[MemTaskNum].TaskName, taskname, len);
							MemLeak_t[MemTaskNum].TaskName[sizeof(MemLeak_t[MemTaskNum].TaskName) - 1] = '\0';
						}
					}
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

#ifdef CONFIG_DMALLOC
	int len = 0;
	int MemTaskNum = 0;
	char *taskname = pcTaskGetName(NULL);
	if (taskname != NULL)
		MemTaskNum = uxTaskGetTaskNumber(xTaskGetHandle(taskname));
#endif

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
		if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
		{
			MemLeak_t[0].FreeSize = pxLink->xBlockSize;
			MemLeak_t[0].FreeTotalSize += pxLink->xBlockSize;
			MemLeak_t[0].FreeCount++;
		}
		else
		{
			if (taskname != NULL)
			{
				MemLeak_t[MemTaskNum].TaskNum = MemTaskNum;
				MemLeak_t[MemTaskNum].FreeSize = pxLink->xBlockSize;
				MemLeak_t[MemTaskNum].FreeTotalSize += pxLink->xBlockSize;
				MemLeak_t[MemTaskNum].FreeCount++;
				len = sizeof(MemLeak_t[MemTaskNum].TaskName) > strlen(taskname) ? strlen(taskname) : sizeof(MemLeak_t[MemTaskNum].TaskName);
				strncpy(MemLeak_t[MemTaskNum].TaskName, taskname, len);
				MemLeak_t[MemTaskNum].TaskName[sizeof(MemLeak_t[MemTaskNum].TaskName) - 1] = '\0';
			}
		}
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
