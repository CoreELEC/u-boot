/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
#include "printk.h"
#if CONFIG_BACKTRACE
#include "stack_trace.h"
#endif
#endif

#define UNWIND_DEPTH 5
#define RAM_REGION_NUMS 2
#define HEAD_CANARY_PATTERN ((size_t)(0x5051525354555657))
#define TAIL_CANARY_PATTERN ((size_t)(0x6061626364656667))
#define HEAD_CANARY(x) ((x)->head_canary)
#define TAIL_CANARY(x, y) \
	(*(size_t *)((size_t)(x) + ((y) & ~xBlockAllocatedBit) - sizeof(size_t)))

/* Used to define the bss and data segments in the program. */
struct tmemory_region {
	size_t *startAddress;
	size_t size;
};

/* Assignment Process Tracker */
struct alloc_trace_block {
	BlockLink_t *allocHandle;
	TaskHandle_t xOwner;
	size_t blockSize;
	size_t requestSize;
	unsigned long backTrace[UNWIND_DEPTH];
};

/* allocation buffer pool tracking alloc */
struct alloc_trace_block allocList[CONFIG_MEMORY_ERROR_DETECTION_SIZE] = {NULL};

#if CONFIG_N200_REVA
// NOTHING
#else
extern uint8_t _bss_start[];
extern uint8_t _bss_len[];
extern uint8_t _data_start[];
extern uint8_t _data_len[];
struct tmemory_region globalRam[RAM_REGION_NUMS] = {
	{(size_t *)_bss_start, (size_t)_bss_len}, {(size_t *)_data_start, (size_t)_data_len}};
#endif

/* For pinpointing the location of anomalies(dump stack) */
// print_traceitem
static void print_traceitem(unsigned long *trace)
{
#if CONFIG_BACKTRACE
	printk("\tCallTrace:\n");
	for (int i = 0; i < UNWIND_DEPTH; i++) {
		printk("\t");
		print_symbol(*(trace + i));
	}
#endif
}
// get_calltrace
static void get_calltrace(unsigned long *trace)
{
#if CONFIG_BACKTRACE
#define CT_SKIP 2
	int32_t ret, i;
	unsigned long _trace[32];

	ret = get_backtrace(NULL, _trace, UNWIND_DEPTH + CT_SKIP);
	if (ret) {
		for (i = 0; i < UNWIND_DEPTH; i++)
			trace[i] = _trace[i + CT_SKIP];
	}
#endif
}
// On-site printing from memory
#ifdef CONFIG_MEMORY_ERROR_DETECTION_PRINT
static void print_memory_site_info(uint8_t *address)
{
#define STEP_VALUE_FOR_MEMORY 8

	int step;

	printk("Memory Request Address Field Details (ADDRESS:0x%08x)\n",
	       (size_t)address);
	printk("\t ADDRESS -%d\n", STEP_VALUE_FOR_MEMORY * sizeof(size_t));

	for (step = (STEP_VALUE_FOR_MEMORY - 1); step >= 0; step--) {
		printk("\t");
		for (int loops = sizeof(size_t); loops > 0; loops--)
			printk("%02x ", *(address - loops - step * sizeof(size_t)));
		printk("\n");
	}

	printk("\t ADDRESS\n");

	for (step = 0; step < STEP_VALUE_FOR_MEMORY; step++) {
		printk("\t");
		for (int loops = 0; loops < sizeof(size_t); loops++)
			printk("%02x ", *(address + loops + step * sizeof(size_t)));
		printk("\n");
	}
}
#endif
/* Additional functions to scan memory (buffer overflow, memory leaks) */
// vPortUpdateFreeBlockList
static void vPortUpdateFreeBlockList(void)
{
	BlockLink_t *start = &xStart;

	do {
		HEAD_CANARY(start) = HEAD_CANARY_PATTERN;
		start = start->pxNextFreeBlock;
	} while (start->pxNextFreeBlock != NULL);
}
// Output out-of-bounds field information
static int xPrintOutOfBoundSite(size_t pos)
{
	int result = 0;
	TaskStatus_t status;

	/* Header integrity check */
	if (HEAD_CANARY(allocList[pos].allocHandle) != HEAD_CANARY_PATTERN) {
		printk("ERROR!!! detected buffer overflow(HEAD)\r\n");

		size_t buffer_address =
		    (size_t)(allocList[pos].allocHandle) + xHeapStructSize;
		size_t buffer_size =
		    allocList[pos].allocHandle->xBlockSize & ~xBlockAllocatedBit;

		if (allocList[pos].xOwner) {
			vTaskGetInfo(allocList[pos].xOwner, &status, 0, 0);
			printk(
			"\tTask owner:(%s) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
			status.pcTaskName, buffer_address, allocList[pos].requestSize, buffer_size);
		} else {
			printk(
			"\tTask owner:(NULL) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
			buffer_address, allocList[pos].requestSize, buffer_size);
		}

		print_traceitem(allocList[pos].backTrace);

#ifdef CONFIG_MEMORY_ERROR_DETECTION_PRINT
		print_memory_site_info((uint8_t *)buffer_address);
#endif
		result++;
	}

	/* Tail integrity check */
	if (TAIL_CANARY(allocList[pos].allocHandle, allocList[pos].blockSize) !=
	    TAIL_CANARY_PATTERN) {
		size_t buffer_address =
		    (size_t)(allocList[pos].allocHandle) + xHeapStructSize;
		size_t buffer_size =
		    allocList[pos].allocHandle->xBlockSize & ~xBlockAllocatedBit;

		printk("ERROR!!! detected buffer overflow(TAIL)\r\n");

		if (allocList[pos].xOwner) {
			vTaskGetInfo(allocList[pos].xOwner, &status, 0, 0);
			printk(
			"\tTask owner:(%s) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
			status.pcTaskName, buffer_address, allocList[pos].requestSize, buffer_size);
		} else {
			printk(
			"\tTask owner:(NULL) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
			buffer_address, allocList[pos].requestSize, buffer_size);
		}

		print_traceitem(allocList[pos].backTrace);

#ifdef CONFIG_MEMORY_ERROR_DETECTION_PRINT
		print_memory_site_info((uint8_t *)buffer_address);
#endif
		result++;
	}

	return result;
}
// output memory leak scene
static int printMemoryLeakSite(size_t pos, size_t allocatedAddress)
{
	TaskStatus_t status;
	size_t buffer_size = allocList[pos].allocHandle->xBlockSize & ~xBlockAllocatedBit;

	printk("WARNING!!! detected buffer leak\r\n");

	if (allocList[pos].xOwner) {
		vTaskGetInfo(allocList[pos].xOwner, &status, 0, 0);
		printk(
		"\tTask owner:(%s) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
		status.pcTaskName, allocatedAddress, allocList[pos].requestSize, buffer_size);
	} else {
		printk(
		"\tTask owner:(NULL) buffer address:(%lx) request size:(%lu) block size:(%lu)\r\n",
		allocatedAddress, allocList[pos].requestSize, buffer_size);
	}

	print_traceitem(allocList[pos].backTrace);
}
// scan dynamic memory is leak?
static int xScanDynamicMemory(size_t pos, size_t allocatedAddress)
{
	size_t *eAdd, *sAdd;

	for (size_t idx = 0; idx < CONFIG_MEMORY_ERROR_DETECTION_SIZE; idx++) {
		if ((pos != idx) && (allocList[idx].allocHandle)) {
			/* Calculate the current thread stack address range */
			sAdd = (size_t *)(xHeapStructSize +
					  (size_t)(allocList[idx].allocHandle));
			eAdd = (size_t *)(allocList[idx].requestSize + (size_t)sAdd);
			/* scan the current memory block */
			for (; sAdd < eAdd; sAdd++) {
				if (*sAdd == allocatedAddress)
					return 1;
			}
		}
	}
	return 0;
}
// Scan static memory blocks
static int xScanStaticMemory(size_t allocatedAddress)
{
	size_t *eAdd, *sAdd;
	size_t *jumpSAdd = (size_t *)(&allocList[0]);
	size_t *jumpEAdd = (size_t *)((size_t)jumpSAdd + sizeof(allocList));

	for (int i = 0; i < RAM_REGION_NUMS; i++) {
		/* Calculate the address range of the static memory area */
		sAdd = globalRam[i].startAddress;
		eAdd = (size_t *)((size_t)(sAdd) + globalRam[i].size);
		/* scan bss & data segment */
		for (; sAdd < eAdd; sAdd++) {
			if ((*sAdd == allocatedAddress) &&
			    ((sAdd < jumpSAdd) || (sAdd > jumpEAdd)))
				return 1;
		}
	}
	return 0;
}
/****************************************************************/
// vPortAddToList
static void vPortAddToList(size_t pointer, size_t tureSize)
{
	size_t pos = 0;
	BlockLink_t *temp = (BlockLink_t *)pointer;

	HEAD_CANARY(temp) = HEAD_CANARY_PATTERN;
	TAIL_CANARY(temp, temp->xBlockSize) = TAIL_CANARY_PATTERN;
	/* Fill Tracking Info Block */
	while (pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE) {
		if (!allocList[pos].allocHandle) {
			allocList[pos].requestSize = tureSize;
			/* cache block size */
			allocList[pos].blockSize = temp->xBlockSize;
			/* mount malloc point */
			allocList[pos].allocHandle = (BlockLink_t *)pointer;
			/* get call stack info */
			get_calltrace(allocList[pos].backTrace);
			/* Current task owener */
			if (xTaskGetCurrentTaskHandle() &&
			    xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
				allocList[pos].xOwner = xTaskGetCurrentTaskHandle();
			} else {
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
	for (pos = 0; pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE; pos++) {
		if (((size_t)(allocList[pos].xOwner)) == allocatedAddress)
			allocList[pos].xOwner = NULL;
	}
	/* Release the specified tracking block */
	for (pos = 0; pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE; pos++) {
		if (allocList[pos].allocHandle == (BlockLink_t *)pointer) {
			allocList[pos].xOwner = NULL;
			allocList[pos].blockSize = 0;
			allocList[pos].requestSize = 0;
			allocList[pos].allocHandle = NULL;
			memset(allocList[pos].backTrace, 0,
			       sizeof(allocList[pos].backTrace));
			break;
		}
	}
}
/****************************************************************/
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

	/* Single node fast detection */
	if ((HEAD_CANARY(allocHandle) != HEAD_CANARY_PATTERN) ||
	    (TAIL_CANARY(allocHandle, allocHandle->xBlockSize) != TAIL_CANARY_PATTERN)) {
		/* Find node buffer pool */
		while (pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE) {
			if (allocList[pos].allocHandle == allocHandle)
				break;
			pos++;
		}

		/* Output out-of-bounds site */
		xPrintOutOfBoundSite(pos);
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

	do {
		configASSERT(HEAD_CANARY(start) == HEAD_CANARY_PATTERN);
		start = start->pxNextFreeBlock;
	} while (start->pxNextFreeBlock != NULL);

	/* Scan allocated memory integrity */
	while (pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE) {
		if (allocList[pos].allocHandle)
			result += xPrintOutOfBoundSite(pos);
		/* Judging whether the inspection is over */
		pos++;
	}

	return result;
}

// Check for orphaned memory(memleak detection)
int xPortMemoryScan(void)
{
	int result = 0;
	size_t found, allocatedAddress;

	/* memory leak scan */
	for (size_t pos = 0; pos < CONFIG_MEMORY_ERROR_DETECTION_SIZE; pos++) {
		/* Scan the specified memory mount point */
		if (allocList[pos].allocHandle) {
			/* update found mark */
			found = 0;
			/* Set target address */
			allocatedAddress = (size_t)(allocList[pos].allocHandle);
			allocatedAddress += xHeapStructSize;
			/* Scan all dynamic memory -1 */
			found = xScanDynamicMemory(pos, allocatedAddress);

			/* Scan all static memory -2 */
#ifndef CONFIG_N200_REVA
			if (!found)
				found = xScanStaticMemory(allocatedAddress);
#endif

			/* Didn't found any references to a pointer -3 */
			if (!found) {
				printMemoryLeakSite(pos, allocatedAddress);
				result++;
			}
		}
	}

	return result;
}
