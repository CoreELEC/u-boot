/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "aml_vsnprintf.h"

int snprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int rv;

	va_start(ap, format);
	rv = vsnprintf(str, size, format, ap);
	va_end(ap);

	return rv;
}

