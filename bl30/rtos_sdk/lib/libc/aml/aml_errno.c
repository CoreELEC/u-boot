/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_errno.h"

static volatile int __aml_errno;

int *__errno(void)
{
	return (int *)&__aml_errno;
}
