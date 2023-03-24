/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdio.h>

/*
 * Here is a good place to include header files
 *  that are required across your application.
 */

#define USER_MODE_TASKS 0

#define configUSE_PREEMPTION 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE 0
#define configCPU_CLOCK_HZ 24000000
#define configTICK_RATE_HZ 50
#define configMAX_PRIORITIES 4
#define configMINIMAL_STACK_SIZE 450
#define configMAX_TASK_NAME_LEN 16
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_TASK_NOTIFICATIONS 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 1
#define configQUEUE_REGISTRY_SIZE 10
#define configUSE_QUEUE_SETS 0
#define configUSE_TIME_SLICING 1
#define configUSE_NEWLIB_REENTRANT 0
#define configENABLE_BACKWARD_COMPATIBILITY 0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configTOTAL_HEAP_SIZE (30 * 1024)
#define configAPPLICATION_ALLOCATED_HEAP 0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK 1
#define configUSE_TICK_HOOK 0
#define configCHECK_FOR_STACK_OVERFLOW 1
#define configUSE_MALLOC_FAILED_HOOK 1
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_TRACE_FACILITY 0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES 0
#define configMAX_CO_ROUTINE_PRIORITIES 1

/* Software timer related definitions. */
#define configUSE_TIMERS 1
#define configTIMER_TASK_PRIORITY 1
#define configTIMER_QUEUE_LENGTH 10
#define configTIMER_TASK_STACK_DEPTH configMINIMAL_STACK_SIZE

/* Interrupt nesting behavior configuration. */
#define configKERNEL_INTERRUPT_PRIORITY 1
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 6
#define configMAX_API_CALL_INTERRUPT_PRIORITY 6

#define configDEFAULT_HEAP_ADDR 0x10000000
#define configDEFAULT_HEAP_SIZE (32 * 1024)
#define configSTICK_REG_ADDR (configDEFAULT_HEAP_ADDR + configDEFAULT_HEAP_SIZE)

#define portCRITICAL_NESTING_IN_TCB 1

/* Define to trap errors during development. */
#define configASSERT(x)                                                                            \
	do {                                                                                       \
		if ((x) == 0) {                                                                    \
			taskDISABLE_INTERRUPTS();                                                  \
			printf("ASSERT: %s %d\n", __FILE__, __LINE__);                             \
			for (;;)                                                                   \
				;                                                                  \
		}                                                                                  \
	} while (0)

/* FreeRTOS MPU specific definitions. */
//#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 0

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_xResumeFromISR 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetIdleTaskHandle 1
#define INCLUDE_eTaskGetState 0
#define INCLUDE_xEventGroupSetBitFromISR 1
#define INCLUDE_xTimerPendFunctionCall 1
#define INCLUDE_xTaskAbortDelay 0
#define INCLUDE_xTaskGetHandle 1
#define INCLUDE_xTaskResumeFromISR 1

/* A header file that defines trace macro can be included here. */

#endif /* FREERTOS_CONFIG_H */
