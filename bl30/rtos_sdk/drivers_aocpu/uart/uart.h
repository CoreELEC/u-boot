/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __UART_H__
#define __UART_H__

#ifdef __cplusplus
extern "C" {
#endif

void vUartInit(void);

int vUartPuts(const char *s);

void vUartTxFlush(void);

void vUartPutc(const char c);

void vUartTxStart(void);

void vUartTxStop(void);

long lUartTxReady(void);

void vUartWriteChar(char c);

void vUart_Debug(void);

void vUartChangeBaudrate_suspend(unsigned long source, unsigned long baud);

void vUartChangeBaudrate_resume(unsigned long baud);

#ifdef __cplusplus
}
#endif
#endif
