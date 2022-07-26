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
extern void vUartInit(void);
extern void vUartPuts(const char *s);
extern void vUartTxFlush(void);
extern void vUartPutc(const char c);
extern void vUartTxStart(void);
extern void vUartTxStop(void);
extern long lUartTxReady(void);
extern void vUartWriteChar(char c);
extern void vUart_Debug(void);
extern void vUartChangeBaudrate_suspend(unsigned long source, unsigned long baud);
extern void vUartChangeBaudrate_resume(unsigned long baud);

#ifdef __cplusplus
}
#endif
#endif
