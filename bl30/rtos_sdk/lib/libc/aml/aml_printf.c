/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include "aml_printf.h"
#include "aml_vsnprintf.h"

#if (1 == CONFIG_ARM64)
#include "serial.h"
#else
#include "uart.h"
#endif

#define MAX_BUFFER_LEN 512

static char printbuffer[MAX_BUFFER_LEN];

int printf(const char *fmt, ...)
{
	va_list args;
	char *p = printbuffer;
	int n = 0;

	va_start(args, fmt);
	vsnprintf(p, MAX_BUFFER_LEN, fmt, args);
	va_end(args);

	while (*p) {
		if ('\n' == *p) {
#if (1 == CONFIG_ARM64)
			vSerialPutChar(ConsoleSerial, '\r');
#else
			vUartPutc('\r');
#endif
			n++;
		}
#if (1 == CONFIG_ARM64)
		vSerialPutChar(ConsoleSerial, *p);
#else
		vUartPutc(*p);
#endif
		n++;
		p++;
	}

	return n;
}

int iprintf(const char *fmt, ...)
{
	va_list args;
	int n = 0;

	va_start(args, fmt);
	n = printf(fmt, args);
	va_end(args);

	return n;
}

int puts(const char *str)
{
#if (1 == CONFIG_ARM64)
	return vSerialPutString(ConsoleSerial, str);
#else
	return vUartPuts(str);
#endif
}
