/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_putchar.h"
#if (1 == CONFIG_ARM64)
#include "serial.h"
#else
#include "uart.h"
#endif

int putchar(int c)
{
#if (1 == CONFIG_ARM64)
	vSerialPutChar(ConsoleSerial, c);
#else
	vUartPutc(c);
#endif

	return 0;
}
