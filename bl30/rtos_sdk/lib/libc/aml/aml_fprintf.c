/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_fprintf.h"
#include <stdarg.h>

int fprintf(FILE *stream, const char *format, ...)
{
	(void)stream;

	va_list args;
	int ret;

	va_start(args, format);
	ret = printf(format, args);
	va_end(args);

	return ret;
}
