/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_strncmp.h"

int strncmp(const char *s1, const char *s2, size_t n)
{
	unsigned char value1;
	unsigned char value2;
	int ret;

	ret = 0;
	for (; n > 0; n--) {
		if ((*s1 != *s2) || (*s1 == '\0')) {
			value1 = *((const unsigned char *)s1);
			value2 = *((const unsigned char *)s2);

			ret = (value1 - value2);
			break;
		}
		s1++;
		s2++;
	}

	return ret;
}
