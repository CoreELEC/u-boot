/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_isalpha.h"
#include "aml_isupper.h"
#include "aml_islower.h"

int isalpha(int c)
{
	return islower(c) || isupper(c);
}
