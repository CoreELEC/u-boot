/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_malloc.h"
#include <FreeRTOS.h>

void *malloc(size_t size)
{
	return pvPortMalloc(size);
}
