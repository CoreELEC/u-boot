/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_strncpy.h"

char *strncpy(char *dest, const char *src, size_t n)
{
	char c;
	char *s = dest;

	--dest;
	if (n >= 4) {
		size_t n4 = n >> 2;

		for (;;) {
			c = *src++;
			*++dest = c;
			if (c == '\0')
				break;
			c = *src++;
			*++dest = c;
			if (c == '\0')
				break;
			c = *src++;
			*++dest = c;
			if (c == '\0')
				break;
			c = *src++;
			*++dest = c;
			if (c == '\0')
				break;
			if (--n4 == 0)
				goto last_chars;
		}
		n = n - (dest - s) - 1;
		if (n == 0)
			return s;
		goto zero_fill;
	}

last_chars:
	n &= 3;
	if (n == 0)
		return s;

	do {
		c = *src++;
		*++dest = c;
		if (--n == 0)
			return s;
	} while (c != '\0');

zero_fill:
	do
		*++dest = '\0';
	while (--n > 0);

	return s;
}
