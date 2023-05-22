/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hw_business.h"
#include "sw_business.h"
#include "gcc_compiler_attributes.h"
#include "board_version.h"

void __weak aocpu_bringup_finished(void)
{
}

int main(void)
{
	printf("Starting AOCPU FreeRTOS\n");
	output_aocpu_info();

	hw_business_process();
	sw_business_process();

#ifdef CONFIG_BL30_VERSION_SAVE
	bl30_plat_save_version();
#endif

	printf("Starting task scheduler ...\n");
	aocpu_bringup_finished();
	vTaskStartScheduler();
	for (;;)

	return 0;
}

void vApplicationIdleHook( void )
{
   //printf("enter idle task\n");

   //write_csr(mie, 1); // open mstatue.mie
   //asm volatile ("wfi"); // enter low power mode
}
/*-----------------------------------------------------------*/
void vApplicationMallocFailedHook( void );

void vApplicationMallocFailedHook( void )
{
    /* The malloc failed hook is enabled by setting
    configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

    Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	//write(1,"malloc failed\n", 14);

	printf("vApplicationMallocFailedHook\n");
	vPrintFreeListAfterMallocFail();
    for ( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    ( void ) pcTaskName;
    ( void ) xTask;

    /* Run time stack overflow checking is performed if
    configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected.  pxCurrentTCB can be
    inspected in the debugger if the task name passed into this function is
    corrupt. */
    //write(1, "Stack Overflow\n", 15);
	printf("vApplicationStackOverflowHook, current task is %s\n", pcTaskName);
	vTaskDumpStack(NULL);
    for ( ;; );
}
/*-----------------------------------------------------------*/
