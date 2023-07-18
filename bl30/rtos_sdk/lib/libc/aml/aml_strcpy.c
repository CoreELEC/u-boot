/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_strcpy.h"

char *strcpy(char *dst, const char *src)
{
	char *dst_tmp = dst;

	while (*src != '\0') {
		*dst_tmp = *src;
		dst_tmp++;
		src++;
	}
	*dst_tmp = *src;

	return dst;
}
