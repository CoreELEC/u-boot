/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include "aml_strncat.h"

char *strncat(char *dest, const char *src, size_t n)
{
	char *ret   = dest;

	dest += strlen(dest);
	for (; n > 0 && *src != '\0' ; n--)
		*dest++ = *src++;

	*dest = '\0';

	return ret;
}
