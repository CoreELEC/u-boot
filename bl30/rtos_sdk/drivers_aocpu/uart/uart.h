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
#include <stdint.h>
#include "gpio.h"

extern void vUartInit(void);

extern int vUartPuts(const char *s);

extern void vUartTxFlush(void);

extern void vUartPutc(const char c);

extern void vUartTxStart(void);

extern void vUartTxStop(void);

extern long lUartTxReady(void);

extern void vUartWriteChar(char c);

extern void vUart_Debug(uint32_t RegBase);

extern void vUartChangeBaudrate_suspend(unsigned long source, unsigned long baud);

extern void vUartChangeBaudrate_resume(unsigned long baud);

extern void vUartWakeupMatchHandler(void);

extern void vUartWakeupNoMatchHandler(void);

extern void vUartWakeupInit(uint16_t GpioRx, uint16_t GpioTx,
			    enum PinMuxType func, uint8_t reinit,
			    void (*vIRHandler)(void), uint32_t baudrate, uint32_t source);

extern void vUartWakeupDeint(void (*vIRHandler)(void));
#ifdef __cplusplus
}
#endif
#endif
