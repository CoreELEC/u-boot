/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_putchar.h"
#include "uart.h"

int putchar(int c)
{
	vUartPutc(c);

	return 0;
}
