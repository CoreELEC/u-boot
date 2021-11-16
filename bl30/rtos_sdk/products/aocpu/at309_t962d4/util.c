/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Utility functions for Chrome EC */

#include "util.h"
#include <string.h>
#include <ctype.h>

int parse_bool(const char *s, int *dest)
{
	if (!strcasecmp(s, "off") || !strncasecmp(s, "dis", 3) ||
	    tolower(*s) == 'f' || tolower(*s) == 'n') {
		*dest = 0;
		return 1;
	} else if (!strcasecmp(s, "on") || !strncasecmp(s, "ena", 3) ||
	    tolower(*s) == 't' || tolower(*s) == 'y') {
		*dest = 1;
		return 1;
	} else {
		return 0;
	}
}

int uint64divmod(uint64_t *n, int d)
{
	uint64_t q = 0, mask;
	int r = 0;

	/* Divide-by-zero returns zero */
	if (!d) {
		*n = 0;
		return 0;
	}

	/* Common powers of 2 = simple shifts */
	if (d == 2) {
		r = *n & 1;
		*n >>= 1;
		return r;
	} else if (d == 16) {
		r = *n & 0xf;
		*n >>= 4;
		return r;
	}

	/* If v fits in 32-bit, we're done. */
	if (*n <= 0xffffffff) {
		uint32_t v32 = *n;
		r = v32 % d;
		*n = v32 / d;
		return r;
	}

	/* Otherwise do integer division the slow way. */
	for (mask = (1ULL << 63); mask; mask >>= 1) {
		r <<= 1;
		if (*n & mask)
			r |= 1;
		if (r >= d) {
			r -= d;
			q |= mask;
		}
	}
	*n = q;
	return r;
}

int __clzsi2(int x)
{
	int r = 0;

	if (!x)
		return 32;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r += 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r += 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r += 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r += 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r += 1;
	}
	return r;
}

int get_next_bit(uint32_t *mask)
{
	int bit = 31 - __clzsi2(*mask);
	*mask &= ~(1 << bit);
	return bit;
}

char *strncpy (char *s1, const char *s2, size_t n)
{
	char c;
	char *s = s1;

	--s1;

	if (n >= 4)
	{
		size_t n4 = n >> 2;

		for (;;)
		{
			c = *s2++;
			*++s1 = c;
			if (c == '\0')
			break;
			c = *s2++;
			*++s1 = c;
			if (c == '\0')
			break;
			c = *s2++;
			*++s1 = c;
			if (c == '\0')
			break;
			c = *s2++;
			*++s1 = c;
			if (c == '\0')
			break;
			if (--n4 == 0)
			goto last_chars;
		}
		n = n - (s1 - s) - 1;
		if (n == 0)
			return s;
		goto zero_fill;
	}

	last_chars:
	n &= 3;
	if (n == 0)
		return s;

	do
	{
		c = *s2++;
		*++s1 = c;
		if (--n == 0)
		return s;
	}
	while (c != '\0');

	zero_fill:
	do
		*++s1 = '\0';
	while (--n > 0);

	return s;
}


