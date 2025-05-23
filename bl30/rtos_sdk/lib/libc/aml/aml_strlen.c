/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_strlen.h"

#if (1 == CONFIG_ARM64)
size_t strlen(const char *s)
#else
int strlen(const char *s)
#endif
{
	int len = 0;

	while (*s++)
		len++;

	return len;
}
