/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_TASKS_EXT_H__
#define __AML_TASKS_EXT_H__

#include <stdint.h>

void vTaskRename(void *pvTaskHandle, const char *pcName);

BaseType_t xTaskSetName(void *pvTaskHandle, const char *pcName);

void vTaskDumpStack(void *pvTaskHandle);

#if (configUSE_TRACE_FACILITY == 1)
void *pvGetTaskHandleOfNum(uint32_t tasknum);
#endif

#if CONFIG_BACKTRACE
void task_stack_range(void *pvTaskHandle, unsigned long *low, unsigned long *high);
#endif

#endif
