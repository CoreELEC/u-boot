/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_tasks_ext.h"

#if (configUSE_TRACE_FACILITY == 1)
static TaskHandle_t prvFindTasksWithinSingleList(List_t *pxList, UBaseType_t tasknum)
{
	configLIST_VOLATILE TCB_t *pxNextTCB, *pxFirstTCB, *pxFound = NULL;

	if (listCURRENT_LIST_LENGTH(pxList) > (UBaseType_t)0) {
		listGET_OWNER_OF_NEXT_ENTRY(pxFirstTCB, pxList);
		do {
			listGET_OWNER_OF_NEXT_ENTRY(pxNextTCB, pxList);
			if (!pxFound && tasknum == pxNextTCB->uxTCBNumber)
				pxFound = pxNextTCB;
		} while (pxNextTCB != pxFirstTCB);
	} else {
		mtCOVERAGE_TEST_MARKER();
	}

	return pxFound;
}
#endif

void vTaskRename(void *pvTaskHandle, const char *pcName)
{
	TCB_t *pxTCB;

	pxTCB = prvGetTCBFromHandle((TaskHandle_t)pvTaskHandle);
	configASSERT(pxTCB);

	memcpy(pxTCB->pcTaskName, pcName, configMAX_TASK_NAME_LEN);
	pxTCB->pcTaskName[configMAX_TASK_NAME_LEN - 1] = '\0';
}

uint8_t pcTaskSetName(void *pvTaskHandle, const char *pcName)
{
	TCB_t *pxTCB;
	UBaseType_t x;
	BaseType_t xReturn;

	/* If null is passed in here then the name of the calling task is being
	queried. */
	pxTCB = prvGetTCBFromHandle((TaskHandle_t)pvTaskHandle);

	/* Store the task name in the TCB. */
	if (pxTCB != NULL && pcName != NULL) {
		for (x = (UBaseType_t)0; x < (UBaseType_t)configMAX_TASK_NAME_LEN; x++) {
			pxTCB->pcTaskName[x] = pcName[x];

			/* Don't copy all configMAX_TASK_NAME_LEN if the string is shorter */
			/* than configMAX_TASK_NAME_LEN characters just in case the memory */
			/* after the string is not accessible (extremely unlikely). */
			if (pcName[x] == (char)0x00)
			{
				/* fix code style waring */
				break;
			}
			else
			{
				/* fix code style waring */
				mtCOVERAGE_TEST_MARKER();
			}
		}

		/* Ensure the name string is terminated in the case that the string length
		was greater or equal to configMAX_TASK_NAME_LEN. */
		pxTCB->pcTaskName[configMAX_TASK_NAME_LEN - 1] = '\0';
	} else
		xReturn = pdFALSE;

	return xReturn;
}

void vTaskDumpStack(void *pvTaskHandle)
{
	TCB_t *pxTCB;
	StackType_t *p;
	int i;

	pxTCB = prvGetTCBFromHandle((TaskHandle_t)pvTaskHandle);
	if (!pxTCB)
		return;
	p = pxTCB->pxStack + pxTCB->uStackDepth - 1;
	printf("Dump Stack:\n");
	while (p >= pxTCB->pxStack) {
		printf("%p:", p);
		for (i = 0; i < 8 && p >= pxTCB->pxStack; i++)
			printf(" %08lx", (long)(*p--));
		printf("\n");
	}
}

#if (configUSE_TRACE_FACILITY == 1)
void *pvGetTaskHandleOfNum(uint32_t tasknum)
{
	UBaseType_t uxQueue = configMAX_PRIORITIES;
	TCB_t *pxFirstTCB = NULL;

	do {
		uxQueue--;
		pxFirstTCB =
		    prvFindTasksWithinSingleList(&(pxReadyTasksLists[uxQueue]), tasknum);
		if (pxFirstTCB)
			goto GetTaskHandleOfNumExit;
	} while (uxQueue > (UBaseType_t)tskIDLE_PRIORITY);
	pxFirstTCB = prvFindTasksWithinSingleList((List_t *)pxDelayedTaskList, tasknum);
	if (pxFirstTCB)
		goto GetTaskHandleOfNumExit;
	pxFirstTCB =
	    prvFindTasksWithinSingleList((List_t *)pxOverflowDelayedTaskList, tasknum);
	if (pxFirstTCB)
		goto GetTaskHandleOfNumExit;
#if (INCLUDE_vTaskDelete == 1)
	{
		pxFirstTCB =
		    prvFindTasksWithinSingleList(&xTasksWaitingTermination, tasknum);
		if (pxFirstTCB)
			goto GetTaskHandleOfNumExit;
	}
#endif

#if (INCLUDE_vTaskSuspend == 1)
	{
		pxFirstTCB = prvFindTasksWithinSingleList(&xSuspendedTaskList, tasknum);
		if (pxFirstTCB)
			goto GetTaskHandleOfNumExit;
	}
#endif
GetTaskHandleOfNumExit:
	return pxFirstTCB;
}
#endif

#if defined(CONFIG_DEBUG_COREDUMP)
// iterate over all threads
void xIterateOverAllThreads(void (*process_task_list)(List_t *list))
{
	int32_t queue = configMAX_PRIORITIES;

	do {
		queue--;
		process_task_list(&pxReadyTasksLists[queue]);
	} while (queue > tskIDLE_PRIORITY);

	process_task_list((List_t *)pxDelayedTaskList);
	process_task_list((List_t *)pxOverflowDelayedTaskList);

#if INCLUDE_vTaskDelete
	process_task_list(&xTasksWaitingTermination);
#endif

#if INCLUDE_vTaskSuspend
	process_task_list(&xSuspendedTaskList);
#endif
}
#endif
