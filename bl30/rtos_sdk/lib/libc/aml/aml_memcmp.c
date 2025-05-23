/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_memcmp.h"

int memcmp(const void *s1, const void *s2, size_t n)
{
	const char *sa = s1;
	const char *sb = s2;
	int diff = 0;

	while (n-- > 0) {
		diff = *(sa++) - *(sb++);
		if (diff)
			return diff;
	}

	return 0;
}
