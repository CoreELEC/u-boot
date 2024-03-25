/*
 * Copyright (C) 2014-2018 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * uart.h
 */

#ifndef __UART_H__
#define __UART_H__

#ifdef __cplusplus
extern "C" {
#endif
#if (ARCH_CPU == RISC_V_N205)
	/* Control uart print by acs flag in bl30 */
	#define ACS_DIS_PRINT_FLAG	(1 << 7)
#ifdef N200_REVA
	#define ACS_DIS_PRINT_REG AO_SEC_GP_CFG7
#else
	#define ACS_DIS_PRINT_REG SYSCTRL_SEC_STATUS_REG4
#endif
	void enable_bl30_print(uint8_t enable);
#if configSUPPORT_STICK_MEM
	void vBL30PrintControlInit(void);
#endif
#endif
	extern void vUartInit(void);
	extern void vUartPuts(const char *s);
	extern void vUartTxFlush(void);
	extern void vUartPutc(const char c);
	extern void vUartTxStart(void);
	extern void vUartTxStop(void);
	extern long lUartTxReady(void);
	extern void vUartWriteChar(char c);

#ifdef __cplusplus
}
#endif
#endif
