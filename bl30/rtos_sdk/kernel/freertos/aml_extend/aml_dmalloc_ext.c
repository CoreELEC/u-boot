/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

struct MemLeak {
	size_t TaskNum;
	size_t WantSize;
	size_t FreeSize;
	size_t WantTotalSize;
	size_t FreeTotalSize;
	size_t MallocCount;
	size_t FreeCount;
	TaskHandle_t xOwner;
};

struct MemLeak MemLeak_t[CONFIG_DMALLOC_SIZE];

void vdRecordMalloc(size_t xWantedSize)
{
	size_t MemTaskNum;
	TaskHandle_t taskTempHandle;

	if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
		MemLeak_t[0].TaskNum = 0;
		MemLeak_t[0].WantSize = xWantedSize;
		MemLeak_t[0].WantTotalSize += xWantedSize;
		MemLeak_t[0].MallocCount++;
		MemLeak_t[0].xOwner = NULL;
	} else {
		taskTempHandle = xTaskGetCurrentTaskHandle();
		MemTaskNum = uxTaskGetTaskNumber(taskTempHandle);
		MemLeak_t[MemTaskNum].TaskNum = MemTaskNum;
		MemLeak_t[MemTaskNum].WantSize = xWantedSize;
		MemLeak_t[MemTaskNum].WantTotalSize += xWantedSize;
		MemLeak_t[MemTaskNum].MallocCount++;
		MemLeak_t[0].xOwner = taskTempHandle;
	}
}

void vdRecordFree(size_t xFreeSize)
{
	size_t MemTaskNum;
	TaskHandle_t taskTempHandle;

	if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
		MemLeak_t[0].FreeCount++;
		MemLeak_t[0].TaskNum = 0;
		MemLeak_t[0].xOwner = NULL;
		MemLeak_t[0].FreeSize = xFreeSize;
		MemLeak_t[0].FreeTotalSize += xFreeSize;
	} else {
		taskTempHandle = xTaskGetCurrentTaskHandle();
		MemTaskNum = uxTaskGetTaskNumber(taskTempHandle);
		MemLeak_t[MemTaskNum].FreeCount++;
		MemLeak_t[MemTaskNum].TaskNum = MemTaskNum;
		MemLeak_t[MemTaskNum].xOwner = taskTempHandle;
		MemLeak_t[MemTaskNum].FreeSize = xFreeSize;
		MemLeak_t[MemTaskNum].FreeTotalSize += xFreeSize;
	}
}

int vPrintDmallocInfo(size_t tid)
{
	int ret = 1;

	printk(
	    "Taskname\tId\tLastmalloc\tLastfree\tTotalmalloc\tTotalfree\tMcount\tFcount\n");

	if (tid == 0) {
		for (int i = 0; i < CONFIG_DMALLOC_SIZE; i++) {
			if (MemLeak_t[i].TaskNum) {
				printk("%-8s\t%-4d\t%-10d\t%-8d\t%-11d\t%-9d\t%-6d\t%d\n",
				       pcTaskGetName(MemLeak_t[i].xOwner),
				       MemLeak_t[i].TaskNum, MemLeak_t[i].WantSize,
				       MemLeak_t[i].FreeSize, MemLeak_t[i].WantTotalSize,
				       MemLeak_t[i].FreeTotalSize,
				       MemLeak_t[i].MallocCount, MemLeak_t[i].FreeCount);
			}
			if (i == 0) {
				printk(
				    "None\t\t%-4d\t%-10d\t%-8d\t%-11d\t%-9d\t%-6d\t%d\n",
				    MemLeak_t[i].TaskNum, MemLeak_t[i].WantSize,
				    MemLeak_t[i].FreeSize, MemLeak_t[i].WantTotalSize,
				    MemLeak_t[i].FreeTotalSize, MemLeak_t[i].MallocCount,
				    MemLeak_t[i].FreeCount);
			}
		}
		ret = 0;
	} else {
		if ((tid >= 0 && tid < CONFIG_DMALLOC_SIZE) && MemLeak_t[tid].TaskNum) {
			printk("%-8s\t%-4d\t%-10d\t%-8d\t%-11d\t%-9d\t%-6d\t%d\n",
			       pcTaskGetName(MemLeak_t[tid].xOwner),
			       MemLeak_t[tid].TaskNum, MemLeak_t[tid].WantSize,
			       MemLeak_t[tid].FreeSize, MemLeak_t[tid].WantTotalSize,
			       MemLeak_t[tid].FreeTotalSize, MemLeak_t[tid].MallocCount,
			       MemLeak_t[tid].FreeCount);
			ret = 0;
		}
	}

	return ret;
}

void xClearSpecDmallocNode(size_t tid)
{
	memset(&MemLeak_t[tid], 0, sizeof(struct MemLeak));
}
