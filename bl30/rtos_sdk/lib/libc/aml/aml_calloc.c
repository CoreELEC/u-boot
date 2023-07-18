/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "aml_calloc.h"
#include <FreeRTOS.h>

void *calloc(size_t nmemb, size_t size)
{
	void *ptr;
	int nb;

	nb = sizeof(size_t) * 4;
	if (size >= SIZE_MAX >> nb || nmemb >= SIZE_MAX >> nb)
		return NULL;

	size *= nmemb;
	ptr = pvPortMalloc(size);
	if (ptr)
		memset(ptr, 0, size);

	return ptr;
}
