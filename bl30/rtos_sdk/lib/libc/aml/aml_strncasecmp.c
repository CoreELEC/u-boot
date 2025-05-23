/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_strncasecmp.h"
#include <ctype.h>

int strncasecmp(const char *s1, const char *s2, size_t n)
{
	int diff;

	if (!n)
		return 0;

	do {
		diff = tolower(*s1) - tolower(*s2);
		if (diff)
			return diff;
	} while (*(s1++) && *(s2++) && --n);

	return 0;
}
