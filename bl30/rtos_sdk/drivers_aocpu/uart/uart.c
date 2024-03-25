/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "common.h"
#include "uart.h"
#include "register.h"
#include "soc.h"
#include "interrupt.h"
#include "suspend.h"
#include <stdio.h>
#ifdef CONFIG_SOC_A4
#include "uart-plat.h"
#endif


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

#define UART_MODE_MASK_2WIRE (1<<15)
#define UART_MODE_MASK_STP_1BIT (0 << 16)
#define UART_MODE_MASK_CHAR_8BIT (0 << 20)
#define UART_MODE_MASK_TX_EN (1 << 12)
#define UART_MODE_MASK_RX_EN (1 << 13)
#define UART_MODE_MASK_RST_TX (1 << 22)
#define UART_MODE_MASK_RST_RX (1 << 23)
#define UART_MODE_MASK_CLR_ERR (1 << 24)
#define UART_CTRL_USE_XTAL_CLK (1 << 24)
#define UART_CTRL_USE_NEW_BAUD_RATE (1 << 23)
#define UART_CTRL_XTAL_CLK_24M (1 << 26)
#define UART_CTRL_XTAL_CLK_DIV2		(1<<27)

#define UART_MISC_CTS_FILTER		(1<<27)
#define UART_MISC_RX_FILTER		(1<<19)

#define UART_STAT_MASK_RFIFO_FULL (1 << 19)
#define UART_STAT_MASK_RX_FIFO_EMPTY (1 << 20)
#define UART_STAT_MASK_TFIFO_FULL (1 << 21)
#define UART_STAT_MASK_TFIFO_EMPTY (1 << 22)

#define P_UART(uart_base, reg) (uart_base + reg)
#define P_UART_WFIFO(uart_base) P_UART(uart_base, UART_WFIFO)
#define P_UART_RFIFO(uart_base) P_UART(uart_base, UART_RFIFO)
#define P_UART_CTRL(uart_base) P_UART(uart_base, UART_CTRL)
#define P_UART_STATUS(uart_base) P_UART(uart_base, UART_STATUS)
#define P_UART_MISC(uart_base) P_UART(uart_base, UART_MISC)
#define P_UART_REG5(uart_base) P_UART(uart_base, UART_REG5)

#ifdef ACS_DIS_PRINT_FLAG
static uint8_t bl30_print_en;
void enable_bl30_print(uint8_t enable)
{
	/* Applied to output important logs */
	bl30_print_en = enable;
}

#ifdef CONFIG_STICK_MEM
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
	while (!(REG32(P_UART_STATUS(UART_PORT_CONS)) & UART_STAT_MASK_TFIFO_EMPTY))
		;
}

void vUartPutc(const char c)
{
#ifdef ACS_DIS_PRINT_FLAG
#ifdef CONFIG_STICK_MEM
	unsigned int stick_mem_bl30_print_en;

	stick_mem_read(STICK_BL30_PRINT_EN, &stick_mem_bl30_print_en);
#endif

	if ((REG32(ACS_DIS_PRINT_REG) & ACS_DIS_PRINT_FLAG) && !bl30_print_en
#ifdef CONFIG_STICK_MEM
		&& (stick_mem_bl30_print_en != STICK_MEM_EN_BL30_PRINT_FLAG)
#endif
	)
		return;
#endif /* ACS_DIS_PRINT_FLAG */

	while (prvUartTxIsFull())
		;
	REG32(P_UART_WFIFO(UART_PORT_CONS)) = (char)c;
	vUartTxFlush();
}

int vUartPuts(const char *s)
{
	int n = 0;

	while (*s) {
		vUartPutc(*s++);
		n++;
	}

	vUartPutc('\r');
	n++;
	vUartPutc('\n');
	n++;

	return n;
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

void vUart_Debug(uint32_t RegBase)
{
	uart_reg[2] = REG32(P_UART_CTRL(RegBase));
	printf("reg2 contrl 0x%x = %x\n", P_UART_CTRL(RegBase), uart_reg[2]);
	uart_reg[3] = REG32(P_UART_STATUS(RegBase));
	printf("reg3 status 0x%x = %x\n", P_UART_STATUS(RegBase), uart_reg[3]);
	uart_reg[4] = REG32(P_UART_MISC(RegBase));
	printf("reg4 misc 0x%x = %x\n", P_UART_MISC(RegBase), uart_reg[4]);
	uart_reg[5] = REG32(P_UART_REG5(RegBase));
	printf("reg5 0x%x = %x\n", P_UART_REG5(RegBase), uart_reg[5]);
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

#ifdef CONFIG_UART_WAKEUP

static uint32_t UartWakeupRegBase = UART_PORT_WAKEUP_REG_BASE;
static uint32_t UartWakeupIrq = UART_PORT_WAKEUP_IRQ;

void vUartWakeupMatchHandler(void)
{
	uint32_t buf[4] = {0};
	int ch;

	while (REG32(P_UART_STATUS(UartWakeupRegBase)) & 0xff) {
		ch = REG32(P_UART_RFIFO(UartWakeupRegBase));
		/* Todo: set the match string,then wake up  sample is "w" */
		printf("%s : ch=%x\n", __func__, ch);
		if ((ch & 0xff) == 'w') {
			buf[0] = UART_RX_WAKEUP;
			STR_Wakeup_src_Queue_Send_FromISR(buf);
			DisableIrq(UartWakeupIrq);
			return;
		}
	}
}

void vUartWakeupInit(uint16_t GpioRx, uint16_t GpioTx,
		     enum PinMuxType func, uint8_t reinit,
		     void (*vIRHandler)(void), uint32_t baudrate, uint32_t source)
{
	int ch;
	uint32_t baud_para = 0;

	/* Todo: set the fix pinmux */
	if (xPinmuxSet(GpioRx, func)) {
		printf("%s :%d uart rx pinmux setting error\n", __func__, __LINE__);
		return;
	}

	if (xPinmuxSet(GpioTx, func)) {
		printf("%s :%d uart tx pinmux setting error\n", __func__, __LINE__);
		return;
	}

	/* todo:  if uart is not use before,you should turn on this code */
	if (reinit) {
		REG32(P_UART_CTRL(UartWakeupRegBase)) = UART_STP_BIT |
							UART_PRTY_BIT |
							UART_CHAR_LEN |
							UART_MODE_MASK_RST_TX |
							UART_MODE_MASK_RST_RX |
							UART_MODE_MASK_CLR_ERR |
							UART_MODE_MASK_TX_EN |
							UART_MODE_MASK_RX_EN |
							UART_MODE_MASK_2WIRE;

		if (source == 24000000) {
			baud_para = (source / 2 + baudrate / 2) / baudrate - 1;
			REG32(P_UART_REG5(UartWakeupRegBase)) = baud_para |
								UART_CTRL_USE_NEW_BAUD_RATE |
								UART_CTRL_USE_XTAL_CLK |
								UART_CTRL_XTAL_CLK_DIV2;
		} else if (source == 32000) {
			baud_para = (source / 2 + baudrate / 2) / baudrate - 1;
			REG32(P_UART_REG5(UartWakeupRegBase)) = baud_para |
								UART_CTRL_USE_NEW_BAUD_RATE |
								UART_CTRL_USE_XTAL_CLK |
								UART_CTRL_XTAL_CLK_DIV2;
		} else {
			baud_para = ((source * 10 / (baudrate * 4) + 5) / 10) - 1;
			/* recommend from VLSI */
			REG32(P_UART_MISC(UartWakeupRegBase)) |= UART_MISC_RX_FILTER |
								 UART_MISC_CTS_FILTER;

			REG32(P_UART_REG5(UartWakeupRegBase)) = baud_para |
								UART_CTRL_USE_NEW_BAUD_RATE;
		}

		ch = REG32(P_UART_CTRL(UartWakeupRegBase));
		ch &= ~(UART_MODE_MASK_RST_TX | UART_MODE_MASK_RST_RX | UART_MODE_MASK_CLR_ERR);
		REG32(P_UART_CTRL(UartWakeupRegBase)) = ch;
	}

	ch = REG32(P_UART_CTRL(UartWakeupRegBase));
	ch |= (1 << 27);
	REG32(P_UART_CTRL(UartWakeupRegBase)) = ch;

	ch = REG32(P_UART_MISC(UartWakeupRegBase));
	ch &= 0xffffff00;
	ch |= (1 << 0);
	REG32(P_UART_MISC(UartWakeupRegBase)) = ch;

	if (vIRHandler) {
		RegisterIrq(UartWakeupIrq, 2, vIRHandler);
		EnableIrq(UartWakeupIrq);
	}

#ifdef CONFIG_SOC_A4
	UART_AO_WakeUp_Setting();
#endif
	/* vUart_Debug(UartWakeupRegBase); */
}

void vUartWakeupDeint(void (*vIRHandler)(void))
{
	if (vIRHandler) {
		DisableIrq(UartWakeupIrq);
		UnRegisterIrq(UartWakeupIrq);
	}

	REG32(P_UART_CTRL(UartWakeupRegBase)) = uart_reg[2];
	REG32(P_UART_STATUS(UartWakeupRegBase)) = uart_reg[3];
	REG32(P_UART_MISC(UartWakeupRegBase)) = uart_reg[4];
	REG32(P_UART_REG5(UartWakeupRegBase)) = uart_reg[5];
}
#endif
