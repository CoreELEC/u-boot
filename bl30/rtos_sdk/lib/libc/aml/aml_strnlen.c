/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_strnlen.h"

#if (1 == CONFIG_ARM64)
size_t strnlen(const char *s, size_t maxlen)
#else
int strnlen(const char *s, int maxlen)
#endif
{
	const char *ss = s;

	while ((maxlen > 0) && *ss) {
		ss++;
		maxlen--;
	}
	return ss - s;
}
