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
 * UART driver
 */

#include "FreeRTOS.h"
#include "timers.h"
#include "common.h"
#include "FreeRTOSConfig.h"
#include "uart.h"
#include "register.h"
#include "soc.h"
#include "string.h"
#include "timer_source.h"

//#define UART_PORT_CONS UART_B_WFIFO

#define UART_STP_BIT UART_MODE_MASK_STP_1BIT
#define UART_PRTY_BIT 0
#define UART_CHAR_LEN   UART_MODE_MASK_CHAR_8BIT
#define UART_MODE_RESET_MASK	\
			(UART_MODE_MASK_RST_TX \
			| UART_MODE_MASK_RST_RX \
			| UART_MODE_MASK_CLR_ERR)

#define UART_WFIFO      (0<<2)
#define UART_RFIFO	(1<<2)
#define UART_MODE	(2<<2)
#define UART_STATUS     (3<<2)
#define UART_IRQCTL	(4<<2)
#define UART_CTRL	(5<<2)
#define UART_MODE_MASK_STP_1BIT                 (0<<16)
#define UART_MODE_MASK_CHAR_8BIT                (0<<20)
#define UART_MODE_MASK_TX_EN                    (1<<12)
#define UART_MODE_MASK_RX_EN                    (1<<13)
#define UART_MODE_MASK_RST_TX                   (1<<22)
#define UART_MODE_MASK_RST_RX                   (1<<23)
#define UART_MODE_MASK_CLR_ERR                  (1<<24)
#define UART_CTRL_USE_XTAL_CLK			(1<<24)
#define UART_CTRL_USE_NEW_BAUD_RATE		(1<<23)

#define UART_STAT_MASK_RFIFO_FULL		(1<<19)
#define UART_STAT_MASK_RFIFO_EMPTY		(1<<20)
#define UART_STAT_MASK_TFIFO_FULL		(1<<21)
#define UART_STAT_MASK_TFIFO_EMPTY		(1<<22)

#define P_UART(uart_base, reg)		(uart_base+reg)
#define P_UART_WFIFO(uart_base)		P_UART(uart_base, UART_WFIFO)
#define P_UART_MODE(uart_base)		P_UART(uart_base, UART_MODE)
#define P_UART_CTRL(uart_base)		P_UART(uart_base, UART_CTRL)
#define P_UART_STATUS(uart_base)	P_UART(uart_base, UART_STATUS)

#define UART_TX_BUF_SIZE        (512)
#define POLL_TX_FIFO_PRD        (portTICK_PERIOD_MS)
#define POLL_TX_BUF_PRD         (portTICK_PERIOD_MS * 10)
#define UART_TXBUF_TASK_PRI     (3)
#define UART_TX_FIFO_FULL_WAIT  (5000) //us

struct xUartTxBuf_t {
	char *pcTxBuf;
	uint32_t ulRdPtr;
	uint32_t ulWrPtr;
	uint32_t ulBufCnt;
	uint32_t ulTimerPeriod;
	TimerHandle_t xTimer;
	uint8_t ucWarnMsgPrtEn;
	uint8_t ucBufReady;
};

static const char cWarnMsg[] = "\r\nWarning: AOCPU log is lost due to TX FIFO full!\n";
static struct xUartTxBuf_t xUartTxBuf;

#ifdef ACS_DIS_PRINT_FLAG
static uint8_t bl30_print_en;
void enable_bl30_print(uint8_t enable)
{
	/* Applied to output important logs */
	bl30_print_en = enable;
}

#if configSUPPORT_STICK_MEM
#include "stick_mem.h"
#include "mailbox-api.h"
/* Applied to enable or disable bl30 start logs before first
 * suspend or shutdown when compile with '--noverbose'.
 */
static void *xMboxBL30PrintEn(void *msg)
{
	stick_mem_write(STICK_BL30_PRINT_EN, *(uint32_t *)msg);

	return NULL;
}

void vBL30PrintControlInit(void)
{
	xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL,
		MBX_CMD_SET_BL30_PRINT, xMboxBL30PrintEn, 0);
}
#endif
#endif /* ACS_DIS_PRINT_FLAG */

static int prvUartTxIsFull(void)
{
	return REG32(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_FULL;
}

void vUartTxFlush(void)
{
	while (!
	       (REG32(P_UART_STATUS(UART_PORT_CONS)) &
		UART_STAT_MASK_TFIFO_EMPTY));
}

static int prvFillTxBuf(char c)
{
	char *pcBuf;

	taskENTER_CRITICAL();
	if (xUartTxBuf.ulBufCnt >= UART_TX_BUF_SIZE) {
		xUartTxBuf.ucWarnMsgPrtEn = 1;
		taskEXIT_CRITICAL();
		return 1;
	}

	pcBuf = xUartTxBuf.pcTxBuf;
	pcBuf[xUartTxBuf.ulWrPtr] = c;
	xUartTxBuf.ulWrPtr++;
	xUartTxBuf.ulBufCnt++;
	if (xUartTxBuf.ulWrPtr >= UART_TX_BUF_SIZE)
		xUartTxBuf.ulWrPtr = 0;
	taskEXIT_CRITICAL();
	return 0;
}

static int prvFillTxBufStr(const char *s)
{
	const char *pcChar = s;
	int iCnt = 0;

	while (*pcChar) {
		if ('\n' == *pcChar) {
			prvFillTxBuf('\r');
			iCnt++;
		}
		prvFillTxBuf(*s);
		iCnt++;
		pcChar++;
	}

	return iCnt;
}

static uint8_t prvChkTxFifoBusy(void)
{
	uint8_t ucIsTxBusy = 0;
	uint32_t time_start;
	uint32_t time_end;
	uint32_t time_elapse;

	/* Check if UART TX FIFO is busy */
	time_start = timere_read_us();
	while (prvUartTxIsFull()) {
		time_end = timere_read_us();
		time_elapse = time_end - time_start;
		if (time_elapse > UART_TX_FIFO_FULL_WAIT) {
			ucIsTxBusy = 1;
			break;
		}
	}

	return ucIsTxBusy;
}

void vUartPutc(const char c)
{
#ifdef ACS_DIS_PRINT_FLAG
#if configSUPPORT_STICK_MEM
	unsigned int stick_mem_bl30_print_en;

	stick_mem_read(STICK_BL30_PRINT_EN, &stick_mem_bl30_print_en);
#endif

	if ((REG32(ACS_DIS_PRINT_REG) & ACS_DIS_PRINT_FLAG) && !bl30_print_en
#if configSUPPORT_STICK_MEM
		&& (stick_mem_bl30_print_en != STICK_MEM_EN_BL30_PRINT_FLAG)
#endif
	)
		return;
#endif /* ACS_DIS_PRINT_FLAG */

	if (c == '\n')
		vUartPutc('\r');

	while (prvUartTxIsFull());
	REG32(P_UART_WFIFO(UART_PORT_CONS)) = (char)c;
}

void vUartPuts(const char *s)
{
	uint8_t ucIsTxBusy;

	while (*s) {
		ucIsTxBusy = prvChkTxFifoBusy();
		/* If Uart TX FIFO is busy, send string to TX Buffer */
		if (ucIsTxBusy && xUartTxBuf.ucBufReady) {
			prvFillTxBufStr(s);
			break;
		}

		if (!ucIsTxBusy || !xUartTxBuf.ucBufReady) {
			vUartPutc(*s);
			s++;
		}
	}
}

void vUartTxStart(void)
{
	/* Do not allow deep sleep while transmit in progress */
#ifdef CONFIG_LOW_POWER_IDLE
	disable_sleep(SLEEP_MASK_UART);
#endif

	//uart_flush_output();
}

void vUartTxStop(void)
{

}

long lUartTxReady(void)
{
	return !(REG32(P_UART_STATUS(UART_PORT_CONS)) &
		 UART_STAT_MASK_TFIFO_FULL);
}
#if 0
void vUartWriteChar(char c)
{
	vUartPutc(c);
}

int uart_tx_char(int c)
{
	vUartPutc(c);

	return c;
}
/*print BCD*/
void print_u32_dec(unsigned int num) {
	char buf[16];
	char *s = buf + (sizeof(buf) / sizeof(buf[0])) - 1;
	char *e = s;

	do {
		*--s = '0' + num % 10;
	} while (num /= 10);

	while (s < e)
		uart_tx_char(*s++);
}

void serial_put_hex(unsigned long data, unsigned int bitlen)
{
	int i;
	unsigned char s;

	for (i = bitlen - 4; i >= 0; i -= 4)
	{
		s = (data >> i) & 0xf;
		if (s < 10)
			vUartPutc(0x30 + s);
		else if (s < 16)
			vUartPutc(0x61 + s - 10);
	}
}
#endif
#if 0
/* Interrupt handler for console UART */
void uart_interrupt(void)
{
	/* Fill output FIFO */
	uart_process_output();
}

DECLARE_IRQ(IRQ_AO_UART_NUM, uart_interrupt, 1);
#endif

static void prvUartTxBufHandle(TimerHandle_t xTimer)
{
	char cTxChar;
	uint32_t ulIsTxFifoEmpty;

	(void)xTimer;
	taskENTER_CRITICAL();
	if (xUartTxBuf.ulBufCnt == 0) {
		if (xUartTxBuf.ulTimerPeriod != POLL_TX_BUF_PRD) {
			xUartTxBuf.ulTimerPeriod = POLL_TX_BUF_PRD;
			xTimerChangePeriod(xUartTxBuf.xTimer, xUartTxBuf.ulTimerPeriod, 0);
		}
		ulIsTxFifoEmpty = (REG32(P_UART_STATUS(UART_PORT_CONS)) &
				UART_STAT_MASK_TFIFO_EMPTY);
		if (ulIsTxFifoEmpty && xUartTxBuf.ucWarnMsgPrtEn) {
			xUartTxBuf.ucWarnMsgPrtEn = 0;
			vUartPuts(cWarnMsg);
		}
	} else {
		if (xUartTxBuf.ulTimerPeriod != POLL_TX_FIFO_PRD) {
			xUartTxBuf.ulTimerPeriod = POLL_TX_FIFO_PRD;
			xTimerChangePeriod(xUartTxBuf.xTimer, xUartTxBuf.ulTimerPeriod, 0);
		}
		while (xUartTxBuf.ulBufCnt) {
			cTxChar = xUartTxBuf.pcTxBuf[xUartTxBuf.ulRdPtr];
			vUartPutc(cTxChar);
			xUartTxBuf.ulRdPtr++;
			xUartTxBuf.ulBufCnt--;
			if (xUartTxBuf.ulRdPtr >= UART_TX_BUF_SIZE)
				xUartTxBuf.ulRdPtr = 0;
		}
	}
	taskEXIT_CRITICAL();
}

/*
 *	Set UART to 115200-8-N-1
 *
 *	Using 24M XTAL as UART reference clock, *NOT* clk81
 *	So the clk81 can be dynamically changed and not
 *	disturb UART transfers.
 */
void vUartInit(void)
{
	TimerHandle_t xUartTimer = NULL;

	xUartTxBuf.pcTxBuf = pvPortMalloc(UART_TX_BUF_SIZE);
	if (xUartTxBuf.pcTxBuf) {
		memset(xUartTxBuf.pcTxBuf, 0, UART_TX_BUF_SIZE);
		xUartTimer = xTimerCreate("UartTimer", pdMS_TO_TICKS(POLL_TX_BUF_PRD),
				pdTRUE, NULL, prvUartTxBufHandle);
		if (xUartTimer) {
			xUartTxBuf.ulRdPtr = 0;
			xUartTxBuf.ulWrPtr = 0;
			xUartTxBuf.ulBufCnt = 0;
			xUartTxBuf.ucWarnMsgPrtEn = 0;
			xUartTxBuf.ucBufReady = 1;
			xUartTxBuf.xTimer = xUartTimer;
			xUartTxBuf.ulTimerPeriod = POLL_TX_BUF_PRD;
			xTimerStart(xUartTxBuf.xTimer, 0);
		} else {
			vPortFree(xUartTxBuf.pcTxBuf);
			vUartPuts("Warning: xUartTimer create fail\n");
		}
	}
}
