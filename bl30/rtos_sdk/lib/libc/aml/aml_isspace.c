/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_isspace.h"

int isspace(int c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
