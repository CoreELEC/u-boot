/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include "aml_strcat.h"

char *strcat(char *dst, const char *src)
{
	(void)strcpy(dst + strlen(dst), src);

	return dst;
}

