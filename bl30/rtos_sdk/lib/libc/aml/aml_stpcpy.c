/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_stpcpy.h"

char *stpcpy(char *dst, const char *src)
{
	while ((*dst++ = *src++) != '\0')
		;	/* nothing */

	return --dst;
}
