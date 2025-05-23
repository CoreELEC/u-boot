/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_isupper.h"

int isupper(int c)
{
	return c >= 'A' && c <= 'Z';
}
