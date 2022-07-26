/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "common.h"
#include "uart.h"
#include "register.h"
#include "soc.h"
#include <stdio.h>


//#define UART_PORT_CONS UART_B_WFIFO

#define UART_STP_BIT UART_MODE_MASK_STP_1BIT
#define UART_PRTY_BIT 0
#define UART_CHAR_LEN UART_MODE_MASK_CHAR_8BIT
#define UART_MODE_RESET_MASK                                                                       \
	(UART_MODE_MASK_RST_TX | UART_MODE_MASK_RST_RX | UART_MODE_MASK_CLR_ERR)

#define UART_WFIFO (0 << 2)
#define UART_RFIFO (1 << 2)
#define UART_CTRL (2 << 2)
#define UART_STATUS (3 << 2)
#define UART_MISC (4 << 2)
#define UART_REG5 (5 << 2)
#define UART_MODE_MASK_STP_1BIT (0 << 16)
#define UART_MODE_MASK_CHAR_8BIT (0 << 20)
#define UART_MODE_MASK_TX_EN (1 << 12)
#define UART_MODE_MASK_RX_EN (1 << 13)
#define UART_MODE_MASK_RST_TX (1 << 22)
#define UART_MODE_MASK_RST_RX (1 << 23)
#define UART_MODE_MASK_CLR_ERR (1 << 24)
#define UART_CTRL_USE_XTAL_CLK (1 << 24)
#define UART_CTRL_USE_NEW_BAUD_RATE (1 << 23)
#define UART_CTRL_XTAL_CLK_DIV2		(1<<27)

#define UART_MISC_CTS_FILTER		(1<<27)
#define UART_MISC_RX_FILTER		(1<<19)

#define UART_STAT_MASK_RFIFO_FULL (1 << 19)
#define UART_STAT_MASK_RX_FIFO_EMPTY (1 << 20)
#define UART_STAT_MASK_TFIFO_FULL (1 << 21)
#define UART_STAT_MASK_TFIFO_EMPTY (1 << 22)

#define P_UART(uart_base, reg) (uart_base + reg)
#define P_UART_WFIFO(uart_base) P_UART(uart_base, UART_WFIFO)
#define P_UART_REG5(uart_base) P_UART(uart_base, UART_REG5)
#define P_UART_CTRL(uart_base) P_UART(uart_base, UART_CTRL)
#define P_UART_STATUS(uart_base) P_UART(uart_base, UART_STATUS)
#define P_UART_MISC(uart_base) P_UART(uart_base, UART_MISC)

static int prvUartTxIsFull(void)
{
	return REG32(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_FULL;
}

void vUartTxFlush(void)
{
	while (!(REG32(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_EMPTY))
		;
}

void vUartPutc(const char c)
{
	if (c == '\n')
		vUartPutc('\r');

	while (prvUartTxIsFull())
		;
	REG32(P_UART_WFIFO(UART_PORT_CONS)) = (char)c;
	vUartTxFlush();
}

void vUartPuts(const char *s)
{
	while (*s)
		vUartPutc(*s++);
}

void vUartTxStart(void)
{
	/* Do not allow deep sleep while transmit in progress */
#ifdef CONFIG_LOW_POWER_IDLE
	disable_sleep(SLEEP_MASK_UART);
#endif

	//uart_flush_output();
}

int uart_reg[6];

void vUart_Debug(void)
{
	uart_reg[2] = REG32(P_UART_CTRL(UART_PORT_CONS));
	printf("reg2 contrl 0x%x = %x\n", P_UART_CTRL(UART_PORT_CONS), uart_reg[2]);
	uart_reg[3] = REG32(P_UART_STATUS(UART_PORT_CONS));
	printf("reg3 status 0x%x = %x\n", P_UART_STATUS(UART_PORT_CONS), uart_reg[3]);
	uart_reg[4] = REG32(P_UART_MISC(UART_PORT_CONS));
	printf("reg4 misc 0x%x = %x\n", P_UART_MISC(UART_PORT_CONS), uart_reg[4]);
	uart_reg[5] = REG32(P_UART_REG5(UART_PORT_CONS));
	printf("reg5 0x%x = %x\n", P_UART_REG5(UART_PORT_CONS), uart_reg[5]);
}

void vUartChangeBaudrate_suspend(unsigned long source, unsigned long baud)
{
	unsigned long baud_para = 0;

	while (!(REG32(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_RX_FIFO_EMPTY))
		;

	if (source == 24000000) {
		baud_para = (source / 2 + baud / 2) / baud - 1;
		REG32(P_UART_REG5(UART_PORT_CONS)) = baud_para | UART_CTRL_USE_NEW_BAUD_RATE |
				UART_CTRL_USE_XTAL_CLK | UART_CTRL_XTAL_CLK_DIV2;
	} else {
		baud_para = ((source * 10 / (baud * 4) + 5) / 10) - 1;
		/* recommend from VLSI */
		REG32(P_UART_MISC(UART_PORT_CONS)) =  REG32(P_UART_MISC(UART_PORT_CONS)) |
				(UART_MISC_RX_FILTER | UART_MISC_CTS_FILTER);

		REG32(P_UART_REG5(UART_PORT_CONS)) = baud_para | UART_CTRL_USE_NEW_BAUD_RATE;

	}
}

void vUartChangeBaudrate_resume(unsigned long baud)
{
	unsigned long baud_para = 0;

	while (!(REG32(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_RX_FIFO_EMPTY))
		;

	REG32(P_UART_MISC(UART_PORT_CONS)) =  REG32(P_UART_MISC(UART_PORT_CONS)) &
			(~(UART_MISC_RX_FILTER | UART_MISC_CTS_FILTER));

	baud_para = (12000000 + baud / 2) / baud - 1;
	REG32(P_UART_REG5(UART_PORT_CONS)) = baud_para | UART_CTRL_USE_XTAL_CLK |
			UART_CTRL_USE_NEW_BAUD_RATE | UART_CTRL_XTAL_CLK_DIV2;

}

void vUartTxStop(void)
{
}

long lUartTxReady(void)
{
	return !(REG32(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_FULL);
}

/*
 *	Set UART to 115200-8-N-1
 *
 *	Using 24M XTAL as UART reference clock, *NOT* clk81
 *	So the clk81 can be dynamically changed and not
 *	diturb UART transfers.
 */
void vUartInit(void)
{
}
