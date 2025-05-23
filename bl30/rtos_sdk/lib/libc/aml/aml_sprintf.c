/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "aml_vsnprintf.h"

#define DEFAULT_BUFFER_SIZE	512

int sprintf(char *str, const char *format, ...)
{
	va_list ap;
	int rv;

	va_start(ap, format);
	rv = vsnprintf(str, DEFAULT_BUFFER_SIZE, format, ap);
	va_end(ap);

	return rv;
}

