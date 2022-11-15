/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_errno.h"
#include "aml_impure.h"

int *__errno(void)
{
	return (int *)&(_impure_ptr->_errno);
}
