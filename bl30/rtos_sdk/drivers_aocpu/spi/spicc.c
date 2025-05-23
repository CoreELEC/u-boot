/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "FreeRTOS.h"
#include "common.h"
#ifdef HIFI
#include "xtensa_api.h"
#endif
#include "queue.h"
#include "semphr.h"
#include "task.h"
#if ENABLE_CPULOAD
#include "cpuload.h"
#endif
#include "spi.h"
#ifdef SOC_AZP1
#include "interrupts.h"
#endif
#ifdef CONFIG_RISCV
#include "n200_func.h"
#include "timer_source.h"
#endif

/* Register Map */
#define SPICC_RXDATA	0x00

#define SPICC_TXDATA	0x04

#define SPICC_CONREG	0x08
#define SPICC_ENABLE		BIT(0)
#define SPICC_MODE_MASTER	BIT(1)
#define SPICC_XCH		BIT(2)
#define SPICC_SMC		BIT(3)
#define SPICC_POL		BIT(4)
#define SPICC_PHA		BIT(5)
#define SPICC_SSCTL		BIT(6)
#define SPICC_SSPOL		BIT(7)
#define SPICC_DRCTL_MASK	GENMASK(9, 8)
#define SPICC_DRCTL_IGNORE	0
#define SPICC_DRCTL_FALLING	1
#define SPICC_DRCTL_LOWLEVEL	2
#define SPICC_CS_MASK		GENMASK(13, 12)
#define SPICC_DATARATE_MASK	GENMASK(18, 16)
#define SPICC_DATARATE_SHIFT 16
#define SPICC_DATARATE_WIDTH 3
#define SPICC_DATARATE_DIV4	0
#define SPICC_DATARATE_DIV8	1
#define SPICC_DATARATE_DIV16	2
#define SPICC_DATARATE_DIV32	3
#define SPICC_BITLENGTH_MASK	GENMASK(24, 19)
#define SPICC_BURSTLENGTH_MASK	GENMASK(31, 25)

#define SPICC_INTREG	0x0c
#define SPICC_TE_EN	BIT(0) /* TX FIFO Empty Interrupt */
#define SPICC_TH_EN	BIT(1) /* TX FIFO Half-Full Interrupt */
#define SPICC_TF_EN	BIT(2) /* TX FIFO Full Interrupt */
#define SPICC_RR_EN	BIT(3) /* RX FIFO Ready Interrupt */
#define SPICC_RH_EN	BIT(4) /* RX FIFO Half-Full Interrupt */
#define SPICC_RF_EN	BIT(5) /* RX FIFO Full Interrupt */
#define SPICC_RO_EN	BIT(6) /* RX FIFO Overflow Interrupt */
#define SPICC_TC_EN	BIT(7) /* Transfer Complete Interrupt */
#define SPICC_TXFIFO_THRESHOLD_EN	BIT(8) /* TX FIFO threshold */
#define SPICC_RXFIFO_THRESHOLD_EN	BIT(9) /* RX FIFO threshold */
#define SPICC_RECV_TOTAL_EN		BIT(10)/* total data received */
#define SPICC_SEND_TOTAL_EN		BIT(11)/* total data send */
#define SPICC_DMA_DONE_EN		BIT(12) /* DMA done */
#define SPICC_RE_EN			BIT(13) /* RX FIFO empty */

#define SPICC_DMAREG	0x10
#define SPICC_DMA_ENABLE		BIT(0)
#define SPICC_TXFIFO_THRESHOLD_MASK	GENMASK(5, 1)
#define SPICC_TXFIFO_THRESHOLD_DEFAULT	10
#define SPICC_RXFIFO_THRESHOLD_MASK	GENMASK(10, 6)
#define SPICC_READ_BURST_MASK		GENMASK(14, 11)
#define SPICC_WRITE_BURST_MASK		GENMASK(18, 15)
#define DMA_REQ_DEFAULT			8
#define SPICC_DMA_URGENT		BIT(19)
#define SPICC_DMA_THREADID_MASK		GENMASK(25, 20)
#define SPICC_DMA_BURSTNUM_MASK		GENMASK(31, 26)

#define SPICC_STATREG	0x14
#define SPICC_TE	BIT(0) /* TX FIFO Empty Interrupt */
#define SPICC_TH	BIT(1) /* TX FIFO Half-Full Interrupt */
#define SPICC_TF	BIT(2) /* TX FIFO Full Interrupt */
#define SPICC_RR	BIT(3) /* RX FIFO Ready Interrupt */
#define SPICC_RH	BIT(4) /* RX FIFO Half-Full Interrupt */
#define SPICC_RF	BIT(5) /* RX FIFO Full Interrupt */
#define SPICC_RO	BIT(6) /* RX FIFO Overflow Interrupt */
#define SPICC_TC	BIT(7) /* Transfer Complete Interrupt */
#define SPICC_TXFIFO_THRESHOLD		BIT(8) /* TX FIFO threshold */
#define SPICC_RXFIFO_THRESHOLD		BIT(9) /* RX FIFO threshold */
#define SPICC_RECV_TOTAL_TRIG		BIT(10)/* total data received */
#define SPICC_SEND_TOTAL_TRIG		BIT(11)/* total data send */
#define SPICC_DMA_DONE			BIT(12) /* DMA done */
#define SPICC_RE			BIT(13) /* RX FIFO empty */

#define SPICC_PERIODREG	0x18
#define SPICC_PERIOD	GENMASK(14, 0)	/* Wait cycles */

#define SPICC_TESTREG	0x1c
#define SPICC_TXCNT_MASK	GENMASK(4, 0)	/* TX FIFO Counter */
#define SPICC_RXCNT_MASK	GENMASK(9, 5)	/* RX FIFO Counter */
#define SPICC_SMSTATUS_MASK	GENMASK(12, 10)	/* State Machine Status */
#define SPICC_LBC		BIT(14) /* Loop Back Control */
#define SPICC_SWAP		BIT(15) /* RX FIFO Data Swap */
#define SPICC_MO_DELAY_MASK	GENMASK(17, 16) /* Master Output Delay */
#define SPICC_MO_NO_DELAY	0
#define SPICC_MO_DELAY_1_CYCLE	1
#define SPICC_MO_DELAY_2_CYCLE	2
#define SPICC_MO_DELAY_3_CYCLE	3
#define SPICC_MI_DELAY_MASK	GENMASK(19, 18) /* Master Input Delay */
#define SPICC_MI_NO_DELAY	0
#define SPICC_MI_DELAY_1_CYCLE	1
#define SPICC_MI_DELAY_2_CYCLE	2
#define SPICC_MI_DELAY_3_CYCLE	3
#define SPICC_MI_DELAY_MIN	SPICC_MI_NO_DELAY
#define SPICC_MI_DELAY_MAX	SPICC_MI_DELAY_3_CYCLE
#define SPICC_MI_CAP_DELAY_MASK	GENMASK(21, 20) /* Master Capture Delay */
#define SPICC_CAP_AHEAD_2_CYCLE	0
#define SPICC_CAP_AHEAD_1_CYCLE	1
#define SPICC_CAP_NO_DELAY	2
#define SPICC_CAP_DELAY_1_CYCLE	3
#define SPICC_CAP_DELAY_MIN	(-2)
#define SPICC_CAP_DELAY_MAX	1
#define SPICC_DELAY_MASK	GENMASK(21, 16)
#define SPICC_FIFORST_MASK	GENMASK(23, 22) /* FIFO Softreset */

#define SPICC_DRADDR	0x20	/* Read Address of DMA */

#define SPICC_DWADDR	0x24	/* Write Address of DMA */

#define SPICC_LD_CNTL0	0x28
#define VSYNC_IRQ_SRC_SELECT		BIT(0)
#define DMA_EN_SET_BY_VSYNC		BIT(2)
#define XCH_EN_SET_BY_VSYNC		BIT(3)
#define DMA_READ_COUNTER_EN		BIT(4)
#define DMA_WRITE_COUNTER_EN		BIT(5)
#define DMA_RADDR_LOAD_BY_VSYNC		BIT(6)
#define DMA_WADDR_LOAD_BY_VSYNC		BIT(7)
#define DMA_ADDR_LOAD_FROM_LD_ADDR	BIT(8)

#define SPICC_LD_CNTL1	0x2c
#define DMA_READ_COUNTER		GENMASK(15, 0)
#define DMA_WRITE_COUNTER		GENMASK(31, 16)
#define SMC_REQ_CNT_MAX		0xffff
#define DMA_BURST_MAX		(DMA_REQ_DEFAULT * SMC_REQ_CNT_MAX)
#define SPICC_DMA_BYTES_PER_WORD	8

#define SPICC_LD_RADDR	0x30

#define SPICC_LD_WADDR	0x34

#define SPICC_ENH_CTL0	0x38	/* Enhanced Feature */
#define SPICC_ENH_CLK_CS_DELAY_MASK	GENMASK(15, 0)
#define SPICC_ENH_DATARATE_MASK		GENMASK(23, 16)
#define SPICC_ENH_DATARATE_SHIFT	16
#define SPICC_ENH_DATARATE_WIDTH	8
#define SPICC_ENH_DATARATE_EN		BIT(24)
#define SPICC_ENH_MOSI_OEN		BIT(25)
#define SPICC_ENH_CLK_OEN		BIT(26)
#define SPICC_ENH_CS_OEN		BIT(27)
#define SPICC_ENH_CLK_CS_DELAY_EN	BIT(28)
#define SPICC_ENH_MAIN_CLK_AO		BIT(29)

#define SPICC_ENH_CTL1	0x3c	/* Enhanced Feature 1 */
#define SPICC_ENH_MI_CAP_DELAY_EN	BIT(0)
#define SPICC_ENH_MI_CAP_DELAY_MASK	GENMASK(9, 1)
#define SPICC_ENH_SI_CAP_DELAY_EN	BIT(14)	/* slave mode */
#define SPICC_ENH_DELAY_EN		BIT(15)
#define SPICC_ENH_SI_DELAY_EN		BIT(16)	/* slave mode */
#define SPICC_ENH_SI_DELAY_MASK		GENMASK(19, 17)	/* slave mode */
#define SPICC_ENH_MI_DELAY_EN		BIT(20)
#define SPICC_ENH_MI_DELAY_MASK		GENMASK(23, 21)
#define SPICC_ENH_MO_DELAY_EN		BIT(24)
#define SPICC_ENH_MO_DELAY_MASK		GENMASK(27, 25)
#define SPICC_ENH_MO_OEN_DELAY_EN	BIT(28)
#define SPICC_ENH_MO_OEN_DELAY_MASK	GENMASK(31, 29)

#define SPICC_ENH_CTL2	0x40	/* Enhanced Feature */
#define SPICC_ENH_TT_DELAY_MASK		GENMASK(14, 0)
#define SPICC_ENH_TT_DELAY_EN		BIT(15)
#define SPICC_ENH_TI_DELAY_MASK		GENMASK(30, 16)
#define SPICC_ENH_TI_DELAY_EN		BIT(31)

#define SPICC_ENH_CTL3	0x44
#define SPICC_ENH_TXFIFO_THRESHOLD	GENMASK(4, 0)
#define SPICC_ENH_RXFIFO_THRESHOLD	GENMASK(9, 5)
#define SPICC_ENH_SLAVE_MODE		BIT(10)
#define SPICC_ENH_WORD_MODE		GENMASK(12, 11)
#define WORD_MODE_8_BYTE		0
#define WORD_MODE_4_BYTE		1
#define WORD_MODE_2_BYTE		2
#define WORD_MODE_1_BYTE		3
#define SPICC_ENH_LAST_TRIG_BY_SS	BIT(13)

#define SPICC_ENH_CTL4	0x48
#define SPICC_ENH_RECV_THRESHOLD	GENMASK(15, 0)
#define SPICC_ENH_SEND_THRESHOLD	GENMASK(31, 16)

#define SPICC_ENH_CTL5	0x4c
#define SPICC_ENH_ENDIAN_TXFIFO	GENMASK(23, 0)

#define SPICC_ENH_CTL6	0x50
#define SPICC_ENH_ENDIAN_RXFIFO	GENMASK(23, 0)
/* for DMA */
#define LITTLE_ENDIAN_1 \
	((0 << 21) | (1 << 18) | (2 << 15) | (3 << 12) \
	| (4 << 9) | (5 << 6) | (6 << 3) | (7 << 0))
#define LITTLE_ENDIAN_2 \
	((1 << 21) | (0 << 18) | (3 << 15) | (2 << 12) \
	| (5 << 9) | (4 << 6) | (7 << 3) | (6 << 0))
#define LITTLE_ENDIAN_4 \
	((3 << 21) | (2 << 18) | (1 << 15) | (0 << 12) \
	| (7 << 9) | (6 << 6) | (5 << 3) | (4 << 0))
#define LITTLE_ENDIAN_8 \
	((7 << 21) | (6 << 18) | (5 << 15) | (4 << 12) \
	| (3 << 9) | (2 << 6) | (1 << 3) | (0 << 0))
/* for PIO */
#define BIG_ENDIAN_1	LITTLE_ENDIAN_8
#define BIG_ENDIAN_2 \
	((6 << 21) | (7 << 18) | (4 << 15) | (5 << 12) \
	| (2 << 9) | (3 << 6) | (0 << 3) | (1 << 0))
#define BIG_ENDIAN_4 \
	((4 << 21) | (5 << 18) | (6 << 15) | (7 << 12) \
	| (0 << 9) | (1 << 6) | (2 << 3) | (3 << 0))
#define BIG_ENDIAN_8	LITTLE_ENDIAN_1

#define SPICC_ENH_STATREG	0x54
#define SPICC_ENH_RECV_TOTAL	GENMASK(15, 0)
#define SPICC_ENH_SEND_TOTAL	GENMASK(31, 16)

#define SPICC_REGS_END		(SPICC_ENH_STATREG + 4)

//#define SPICC_HW_DEBUG

struct SpiccHwCompatibleData {
	u32 min_speed_hz;
	u32 max_speed_hz;
	u32 fifo_size;
	u32 dma_burst_triggered_by_ssctl:1;
	u32 has_oen:1;
	u32 has_enhance_clk_div:1;
	u32 has_cs_pre_delay:1;
	u32 has_enhance_io_delay:1;
	u32 has_comp_clk:1;
	u32 is_div_parent_comp_clk:1;
	u32 has_enhance_tt_ti_delay:1;
	u32 has_word_mode_ctrl:1;
	u32 has_endian_ctrl:1;
	u32 has_enh_intr:1;
	u32 support_dma_burst_len_1:1;
};

struct SpiccHw {
	const struct SpiccHwCompatibleData *data;
	unsigned long base;
	int irq;
	int is_slave;
	int force_ssctl;
	u32 clk_div;
	SemaphoreHandle_t xXferSemaphore;
	u32 clk_rate;
	u32 speed_hz;
	u16 mode;
	u8 bits_per_word;
	u8 bytes_per_word;
	const u8 *tx_buf;
	u8 *rx_buf;
	u32 tx_remain;
	u32 rx_remain;
	u32 using_dma:1;
#if ENABLE_CPULOAD
	CPULD_tstrME * strME;
#endif
};

#ifndef time_after
#define time_after(a, b) ((long)((b) - (a)) < 0)
#define time_before(a, b) time_after(b, a)
#endif

#ifdef HIFI
#define vSpiccCacheClean(ptr, size) \
	xthal_dcache_region_writeback(((void *)ptr), ((uint32_t)size))
#define vSpiccCacheInv(ptr, size) \
	xthal_dcache_region_invalidate(((void *)ptr), ((uint32_t)size))
#elif defined ARCH64
#define vSpiccCacheClean(ptr, size) \
	vCacheCleanDcacheRange(((uint64_t)ptr), ((uint64_t)size))
#define vSpiccCacheInv(ptr, size) \
	vCacheInvDcacheRange(((uint64_t)ptr), ((uint64_t)size))
#else
#define vSpiccCacheClean(ptr, size) do {} while (0)
#define vSpiccCacheInv(ptr, size) do {} while (0)
#endif

/* reg write/read */
#define SpiccReadl(addr) REG32(addr)
#define SpiccWritel(val, addr) (REG32(addr) = val)

/* bits write/read */
#define SpiccWriteBits(mask, val, addr) \
	REG32_UPDATE_BITS(addr, mask, FIELD_PREP(mask, val))
#define SpiccReadBits(mask, addr) FIELD_GET(mask, REG32(addr))

/* bits set(1)/clr(0) */
#define SpiccSetBits(mask, addr) \
		(REG32(addr) = (REG32(addr)) | (mask))
#define SpiccClrBits(mask, addr) \
		(REG32(addr) = (REG32(addr)) & (~(mask)))

#define SpiccIsBitsTrue(mask, addr) (!!((REG32(addr)) & (mask)))
#define SpiccIsBitsFalse(mask, addr) (!((REG32(addr)) & (mask)))

static const struct SpiccHwCompatibleData SpiccHwCompatibleData[] = {
	[MESON_GXL_SPICC] = {325000, 4166667, 16, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	[MESON_TXL_SPICC] = {325000, 83333333, 16, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	[MESON_AXG_SPICC] = {325000, 83333333, 16, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
	[MESON_G12A_SPICC] = {50000, 166666667, 15, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
	[MESON_S5_SPICC] = {50000, 166666667, 15, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

#ifdef SPICC_HW_DEBUG
static void prvSpiccDumpReg(struct SpiccHw *hw, char *tag)
{
	unsigned long addr = hw->base + SPICC_CONREG;
	int line = 5;

	spi_dbg("spicc dump %s\n", tag);
	while (line--) {
		spi_dbg("0x%08x: %08x %08x %08x %08x\n", (unsigned int)addr,
			(unsigned int)SpiccReadl(addr),
			(unsigned int)SpiccReadl(addr + 0x04),
			(unsigned int)SpiccReadl(addr + 0x08),
			(unsigned int)SpiccReadl(addr + 0x0c));
		addr += 0x10;
	}
}
#else
#define prvSpiccDumpReg(_hw, _tag) do {} while (0)
#endif

static void prvSpiccHwSetSpeed(struct SpiccHw *hw, u32 speed_hz)
{
	u32 div, mid_speed;

	if (hw->data->has_enhance_clk_div) {
		div = hw->clk_rate / speed_hz;
		if (div < 2)
			div = 2;
		div = (div >> 1) - 1;
		if (div > 0xff)
			div = 0xff;
		hw->clk_div = (div + 1) << 1;
		SpiccWriteBits(SPICC_ENH_DATARATE_MASK,
			       div, hw->base + SPICC_ENH_CTL0);
	} else {
		/* speed = sys_clk_rate / 2^(conreg.data_rate_div+2) */
		mid_speed = (hw->clk_rate * 3) >> 4;
		for (div = 0; div < 7; div++) {
			if (speed_hz >= mid_speed)
				break;
			mid_speed >>= 1;
		}
		hw->clk_div = 1 << (div + 2);
		SpiccWriteBits(SPICC_DATARATE_MASK,
			       div, hw->base + SPICC_CONREG);
	}
}

static int xLimitRange(int val, int min, int max)
{
	int ret;

	if (val < min)
		ret = min;
	else if (val > max)
		ret = max;
	else
		ret = val;

	return ret;
}

static void prvSpiccHwSetIoDelay(struct SpiccHw *hw, int latency)
{
	u32 conf;
	int shift, mi_delay, cap_delay;

	shift = (int)(hw->clk_div >> 1) - latency;
	mi_delay = xLimitRange(shift, SPICC_MI_DELAY_MIN, SPICC_MI_DELAY_MAX);
	cap_delay = xLimitRange(mi_delay - shift,
				SPICC_CAP_DELAY_MIN, SPICC_CAP_DELAY_MAX);
	cap_delay += 2;

	conf = SpiccReadl(hw->base + SPICC_TESTREG);
	conf &= ~(SPICC_MO_DELAY_MASK | SPICC_MI_DELAY_MASK
		  | SPICC_MI_CAP_DELAY_MASK);
	conf |= FIELD_PREP(SPICC_MI_DELAY_MASK, mi_delay);
	conf |= FIELD_PREP(SPICC_MI_CAP_DELAY_MASK, cap_delay);
	SpiccWritel(conf, hw->base + SPICC_TESTREG);
}

static void prvSpiccHwSetEndian(struct SpiccHw *hw, BaseType_t big_endian)
{
	uint32_t endian;
	int bytes_per_word = hw->bits_per_word / 8;

	if (!hw->data->has_endian_ctrl) {
		return;
	} else if (hw->using_dma) {
		if (big_endian)
			endian = BIG_ENDIAN_8;
		else if (bytes_per_word == 1)
			endian = LITTLE_ENDIAN_1;
		else if (bytes_per_word == 2)
			endian = LITTLE_ENDIAN_2;
		else if (bytes_per_word == 4)
			endian = LITTLE_ENDIAN_4;
		else if (bytes_per_word == 8)
			endian = LITTLE_ENDIAN_8;
		else
			return;
	} else {
		if (!big_endian)
			endian = LITTLE_ENDIAN_8;
		else if (bytes_per_word == 1)
			endian = BIG_ENDIAN_1;
		else if (bytes_per_word == 2)
			endian = BIG_ENDIAN_2;
		else if (bytes_per_word == 4)
			endian = BIG_ENDIAN_4;
		else if (bytes_per_word == 8)
			endian = BIG_ENDIAN_8;
		else
			return;
	}

	SpiccWritel(endian, hw->base + SPICC_ENH_CTL5);
	SpiccWritel(endian, hw->base + SPICC_ENH_CTL6);
}

static void prvSpiccHwSetWordMode(struct SpiccHw *hw)
{
	uint32_t word_mode, val;
	int bytes_per_word = hw->bits_per_word / 8;

	if (!hw->data->has_word_mode_ctrl)
		return;
	else if (!hw->using_dma)
		word_mode = WORD_MODE_8_BYTE;
	else if (bytes_per_word == 1)
		word_mode = WORD_MODE_1_BYTE;
	else if (bytes_per_word == 2)
		word_mode = WORD_MODE_2_BYTE;
	else if (bytes_per_word == 4)
		word_mode = WORD_MODE_4_BYTE;
	else
		word_mode = WORD_MODE_8_BYTE;

	val = FIELD_PREP(SPICC_ENH_WORD_MODE, word_mode);
	val |= SPICC_ENH_SLAVE_MODE | SPICC_ENH_LAST_TRIG_BY_SS;
	SpiccWritel(val, hw->base + SPICC_ENH_CTL3);
}

static void prvSpiccHwSetMode(struct SpiccHw *hw, u16 mode)
{
	u32 conf = SpiccReadl(hw->base + SPICC_CONREG);

	conf &= ~(SPICC_POL | SPICC_PHA | SPICC_DRCTL_MASK);

	if (mode & SPI_CPOL)
		conf |= SPICC_POL;

	if (mode & SPI_CPHA)
		conf |= SPICC_PHA;

	if (mode & SPI_READY)
		conf |= FIELD_PREP(SPICC_DRCTL_MASK, SPICC_DRCTL_LOWLEVEL);
	else
		conf |= FIELD_PREP(SPICC_DRCTL_MASK, SPICC_DRCTL_IGNORE);

	SpiccWritel(conf, hw->base + SPICC_CONREG);

	if (mode & SPI_LOOP)
		SpiccSetBits(SPICC_LBC, hw->base + SPICC_TESTREG);
	else
		SpiccClrBits(SPICC_LBC, hw->base + SPICC_TESTREG);

	prvSpiccHwSetEndian(hw, (mode & SPI_LSB_FIRST) ? pdTRUE : pdFALSE);
	prvSpiccHwSetWordMode(hw);
}

static void prvSpiccResetFifo(struct SpiccHw *hw, u8 pr)
{
	u32 val;
	int i;

	/* reset tx/rx fifo */
	SpiccSetBits(SPICC_FIFORST_MASK, hw->base + SPICC_TESTREG);
	/* Empty RX FIFO */
	for (i = 0; i < 32; i++) {
		val = SpiccReadl(hw->base + SPICC_RXDATA);
		if (pr)
			spi_dbg("invalid dummy 0x%x\n", val);
	}
}

static int xSpiccHwTx(struct SpiccHw *hw)
{
	unsigned long reg_sta = hw->base + SPICC_STATREG;
	unsigned long reg_txdata = hw->base + SPICC_TXDATA;
	uint32_t data;
	uint8_t *pu8, bytes, shift;
	int remain = hw->tx_remain, count;

	if (!hw->tx_buf) {
		while (remain && SpiccIsBitsFalse(SPICC_TF, reg_sta)) {
			SpiccWritel(0, reg_txdata);
			remain--;
		}
	} else {
		pu8 = (uint8_t *)hw->tx_buf;
		while (remain && SpiccIsBitsFalse(SPICC_TF, reg_sta)) {
			bytes = hw->bytes_per_word;
			data = 0;
			shift = 0;
			/* 0x11, 0x22, 0x33, 0x44 -> 0x44332211 */
			while (bytes--) {
				data |= (*pu8++) << shift;
				shift += 8;
			}
			SpiccWritel(data, reg_txdata);
			remain--;
		}
		hw->tx_buf = pu8;
	}

	count = hw->tx_remain - remain;
	if (count) {
		hw->tx_remain = remain;
		SpiccSetBits(SPICC_XCH, hw->base + SPICC_CONREG);
	}

	return count;
}

static int xSpiccHwRx(struct SpiccHw *hw)
{
	unsigned long reg_sta = hw->base + SPICC_STATREG;
	unsigned long reg_rxdata = hw->base + SPICC_RXDATA;
	uint32_t data;
	uint8_t *pu8, bytes, shift;
	int remain = hw->rx_remain, count;

	if (!hw->rx_buf) {
		while (remain && SpiccIsBitsTrue(SPICC_RR, reg_sta)) {
			data = SpiccReadl(reg_rxdata);
			remain--;
		}
	} else {
		pu8 = hw->rx_buf;
		while (remain && SpiccIsBitsTrue(SPICC_RR, reg_sta)) {
			data = SpiccReadl(reg_rxdata);
			bytes = hw->bytes_per_word;
			shift = 0;
			/* 0x44332211 -> 0x11, 0x22, 0x33, 0x44 */
			while (bytes--) {
				*pu8++ = (uint8_t)(data >> shift);
				shift += 8;
			}
			remain--;
		}
		hw->rx_buf = pu8;
	}

	count = hw->rx_remain - remain;
	hw->rx_remain = remain;

	return count;
}

static int xSpiccHwRxTimeout(struct SpiccHw *hw, int count)
{
	unsigned long timeout = timere_read_us() + 10000;

	do {
		count -= xSpiccHwRx(hw);
		if (!count)
			break;
	} while (time_before(timere_read_us(), timeout));

	return count ? -ETIMEDOUT : 0;
}

static u32 xSpiccHwDmaCalLen(struct SpiccHw *hw, u32 *req)
{
	u32 len = hw->tx_remain;
	u32 i;

	if (len <= hw->data->fifo_size) {
		*req = len;
		return len;
	}

	*req = DMA_REQ_DEFAULT;
	if (len == (DMA_BURST_MAX + 1)) {
		len = DMA_BURST_MAX - DMA_REQ_DEFAULT;
	} else if (len >= DMA_BURST_MAX) {
		len = DMA_BURST_MAX;
	} else {
		/* 1 < len < DMA_BURST_MAX */
		for (i = DMA_REQ_DEFAULT; i > 1; i--) {
			if ((len % i) == 0) {
				*req = i;
				return len;
			}
		}

		if (hw->data->support_dma_burst_len_1) {
			*req = 1;
			return len;
		}
		if ((len % DMA_REQ_DEFAULT) == 1)
			len -= DMA_REQ_DEFAULT;
		len -= len % DMA_REQ_DEFAULT;
	}

	return len;
}
static void prvSpiccHwDmaBurst(struct SpiccHw *hw)
{
	unsigned int words;
	u32 req;
	unsigned int count_en = 0;
	unsigned int txfifo_thres = 0;
	unsigned int read_req = 0;
	unsigned int rxfifo_thres = 31;
	unsigned int write_req = 0;
	unsigned int ld_ctr1 = 0;

	words = xSpiccHwDmaCalLen(hw, &req);

	/* Setup Xfer variables */
	hw->tx_remain -= words;

	words /= req;
	if (hw->tx_buf) {
		count_en |= DMA_READ_COUNTER_EN;
		txfifo_thres = hw->data->fifo_size + 1 - req;
		read_req = req - 1;
		ld_ctr1 |= FIELD_PREP(DMA_READ_COUNTER, words);
	}

	if (hw->rx_buf) {
		count_en |= DMA_WRITE_COUNTER_EN;
		rxfifo_thres = req - 1;
		write_req = req - 1;
		ld_ctr1 |= FIELD_PREP(DMA_WRITE_COUNTER, words);
	}

	/* Enable DMA write/read counter */
	SpiccWritel(count_en, hw->base + SPICC_LD_CNTL0);
	/* Setup burst length */
	SpiccWritel(ld_ctr1, hw->base + SPICC_LD_CNTL1);

	SpiccWritel(SPICC_DMA_ENABLE
		    | SPICC_DMA_URGENT
		    | FIELD_PREP(SPICC_TXFIFO_THRESHOLD_MASK, txfifo_thres)
		    | FIELD_PREP(SPICC_READ_BURST_MASK, read_req)
		    | FIELD_PREP(SPICC_RXFIFO_THRESHOLD_MASK, rxfifo_thres)
		    | FIELD_PREP(SPICC_WRITE_BURST_MASK, write_req),
		    hw->base + SPICC_DMAREG);

	SpiccWritel(hw->speed_hz >> 25, hw->base + SPICC_PERIODREG);
	SpiccSetBits(SPICC_SMC, hw->base + SPICC_CONREG);
	prvSpiccDumpReg(hw, "dma");
}

/**
 * return: 0 - transfer complete,
 *	   -1- busy error
 */
static int xSpiccHwPolling(struct SpiccHw *hw)
{
	int time_out = 20000; /* unit: usec */
	int ret = -ETIMEDOUT;

	while (time_out--) {
		udelay(1);
		if (hw->using_dma) {
			if (SpiccIsBitsFalse(SPICC_DMA_ENABLE,
					     hw->base + SPICC_DMAREG)) {
				ret = 0;
				break;
			}
		}
		if (SpiccIsBitsTrue(SPICC_TC, hw->base + SPICC_STATREG)) {
			SpiccSetBits(SPICC_TC, hw->base + SPICC_STATREG);
			if (!hw->using_dma) {
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

static void vSpiccHandleIsr(void *vArg)
{
	struct SpiccHw *hw = vArg;
	BaseType_t reschedule = pdFALSE;
	int ret;

#if ENABLE_CPULOAD
	CPULD_vidIsrEnter(hw->strME);
#endif

	if (SpiccIsBitsTrue(SPICC_TC, hw->base + SPICC_STATREG))
		SpiccSetBits(SPICC_TC, hw->base + SPICC_STATREG);

	if (hw->using_dma) {
		if (xSpiccHwPolling(hw))
			return;

		if (hw->tx_remain)
			prvSpiccHwDmaBurst(hw);
		else {
			SpiccWritel(0, hw->base + SPICC_INTREG);
			xSemaphoreGiveFromISR(hw->xXferSemaphore, &reschedule);
			portYIELD_FROM_ISR(reschedule);
		}
	} else {
		xSpiccHwRx(hw);
		if (hw->tx_remain)
			xSpiccHwTx(hw);
		else {
			xSpiccHwRxTimeout(hw, hw->rx_remain);
			SpiccWritel(0, hw->base + SPICC_INTREG);
			xSemaphoreGiveFromISR(hw->xXferSemaphore, &reschedule);
			portYIELD_FROM_ISR(reschedule);
		}
	}
#if ENABLE_CPULOAD
	CPULD_vidIsrExit(hw->strME);
#endif
}

#if (defined ARCH_CPU_M4 || defined CONFIG_RISCV)
#define VID_IRQ_SPICC_BASE	1
#define VID_IRQ_SPICC_PRIO	0xAA

#ifdef CONFIG_RISCV
#define NVIC_vidIRQClearLatch(_vid) do {} while (0)
#endif

void vSpiccHandleIsrNoContext(void)
{
	struct SpiccHw *hw = pvSpiMasterGetDevDataByBusNum(0);

	if (hw) {
		vSpiccHandleIsr(hw);
		NVIC_vidIRQClearLatch(VID_IRQ_SPICC_BASE + 0);
	}
}

void vSpiccHandleIsrNoContext1(void)
{
	struct SpiccHw *hw = pvSpiMasterGetDevDataByBusNum(1);

	if (hw) {
		vSpiccHandleIsr(hw);
		NVIC_vidIRQClearLatch(VID_IRQ_SPICC_BASE + 1);
	}
}

void vSpiccHandleIsrNoContext2(void)
{
	struct SpiccHw *hw = pvSpiMasterGetDevDataByBusNum(2);

	if (hw) {
		vSpiccHandleIsr(hw);
		NVIC_vidIRQClearLatch(VID_IRQ_SPICC_BASE + 2);
	}
}

#ifdef CONFIG_RISCV
static function_ptr_t SpiccHandleIsrTable[] = {
	&vSpiccHandleIsrNoContext,
	&vSpiccHandleIsrNoContext1,
	&vSpiccHandleIsrNoContext2
};
#endif
#endif /* end of (defined ARCH_CPU_M4 || defined ARCH_AOCPU) */

static int xSpiccHwXfer(struct SpiccHw *hw, const u8 *tx_buf, u8 *rx_buf, u32 len)
{
	int count;
	int ret = 0;

	hw->tx_buf = tx_buf;
	hw->rx_buf = rx_buf;
	hw->tx_remain = len / (hw->using_dma ? 8 : hw->bytes_per_word);
	hw->rx_remain = hw->tx_remain;

	prvSpiccResetFifo(hw, 0);
	if (hw->using_dma) {
		SpiccWritel((u32)(unsigned long)tx_buf,
						hw->base + SPICC_DRADDR);
		SpiccWritel((u32)(unsigned long)rx_buf,
						hw->base + SPICC_DWADDR);
		if (tx_buf)
			vSpiccCacheClean(tx_buf, len);
		if (rx_buf)
			vSpiccCacheClean(rx_buf, len);
	}

	if (hw->irq) {
		if (hw->using_dma) {
			prvSpiccHwDmaBurst(hw);
			SpiccWritel(hw->data->has_enh_intr ?
				    SPICC_DMA_DONE_EN : SPICC_TE_EN,
				    hw->base + SPICC_INTREG);
			ret = xSemaphoreTake(hw->xXferSemaphore, portMAX_DELAY);
			ret = (ret == pdFALSE) ? -ETIMEDOUT : 0;
		} else {
			xSpiccHwTx(hw);
			SpiccWritel(hw->is_slave ? SPICC_RR_EN : SPICC_TC_EN,
				    hw->base + SPICC_INTREG);
			ret = xSemaphoreTake(hw->xXferSemaphore, portMAX_DELAY);
			if (ret == pdFALSE) {
				prvSpiccDumpReg(hw, "pio-timedout");
				ret = -ETIMEDOUT;
			} else {
				ret = 0;
			}
		}
	}
	/* dma polling */
	else if (hw->using_dma) {
		while (hw->tx_remain) {
			prvSpiccHwDmaBurst(hw);
			ret = xSpiccHwPolling(hw);
			if (ret)
				break;
		}
	}
	/* pio polling */
	else {
		while (hw->tx_remain) {
			count = xSpiccHwTx(hw);
			ret = xSpiccHwRxTimeout(hw, count);
			if (ret)
				break;
		}
	}

	if (hw->using_dma) {
		if (tx_buf)
			vSpiccCacheInv(tx_buf, len);
		if (rx_buf)
			vSpiccCacheInv(rx_buf, len);
	}

	SpiccClrBits(SPICC_SMC, hw->base + SPICC_CONREG);
	SpiccWritel(0, hw->base + SPICC_INTREG);
	SpiccWritel(0, hw->base + SPICC_DMAREG);
	SpiccWritel(0, hw->base + SPICC_LD_CNTL0);
	SpiccWritel(0, hw->base + SPICC_LD_CNTL1);

	return ret;
}

static int xSpiccHwPrepare(struct SpiccHw *hw, u32 speed_hz,
		    u16 mode, u8 bits_per_word, int latency)
{
	if (mode != hw->mode) {
		hw->mode = mode;
		prvSpiccHwSetMode(hw, mode);
	}

	// if there is no latency on MOSI, set slave sampling timing at the
	// middle of spi-clk, otherwise, set it at the end of spi-clk
	if (hw->is_slave)
		SpiccWritel((latency > 0) ?  0 : SPICC_ENH_DELAY_EN |
			SPICC_ENH_SI_CAP_DELAY_EN, hw->base + SPICC_ENH_CTL1);

	else if ((speed_hz != hw->speed_hz)
	    && (speed_hz >= hw->data->min_speed_hz)
	    && (speed_hz <= hw->data->max_speed_hz)) {
		hw->speed_hz = speed_hz;
		prvSpiccHwSetSpeed(hw, speed_hz);
		prvSpiccHwSetIoDelay(hw, latency);
	}

	if ((bits_per_word != hw->bits_per_word)
	    && (bits_per_word > 0) && (bits_per_word <= 64)) {
		hw->bits_per_word = bits_per_word;
		hw->bytes_per_word = ((bits_per_word - 1) >> 3) + 1;
		SpiccWriteBits(SPICC_BITLENGTH_MASK, bits_per_word - 1,
			       hw->base + SPICC_CONREG);
	}

	hw->using_dma = (bits_per_word == 64) ? 1 : 0;

	return 0;
}

static int xSpiccHwUnprepare(struct SpiccHw *hw)
{
	/* Disable all IRQs */
	SpiccWritel(0, hw->base + SPICC_INTREG);

	/* Disable DMA */
	SpiccWritel(0, hw->base + SPICC_DMAREG);

	hw->using_dma = 0;

	return 0;
}

#if defined(HIFI)
#define VID_IRQ_SPICC_BASE      1
#define VID_IRQ_SPICC_PRIO      0xAA
#endif

static void vSpiccHwInit(struct SpiccHw *hw, u8 compatible, int bus_num)
{
	u32 val;
#if ENABLE_CPULOAD
	char name[5] = {'s', 'p', 'i', '0', 0};
#endif

	hw->data = &SpiccHwCompatibleData[compatible];

	/* Enable Master */
	val = SPICC_ENABLE | (hw->is_slave ? 0 : SPICC_MODE_MASTER);
	val |= SPICC_BURSTLENGTH_MASK;
	if (hw->data->dma_burst_triggered_by_ssctl || hw->force_ssctl)
		val |= SPICC_SSCTL;
	SpiccWritel(val, hw->base + SPICC_CONREG);

	/* Enable output */
	if (!hw->is_slave && hw->data->has_oen)
		SpiccSetBits(SPICC_ENH_MOSI_OEN
			     | SPICC_ENH_CLK_OEN
			     | SPICC_ENH_CS_OEN
			     | SPICC_ENH_MAIN_CLK_AO
			     | SPICC_ENH_DATARATE_EN,
			     hw->base + SPICC_ENH_CTL0);

	hw->xXferSemaphore = xSemaphoreCreateBinary();
	if (!hw->xXferSemaphore)
		spi_err("SPICC Create semaphore failed\n");
	else if (hw->irq) {
#if ENABLE_CPULOAD
		name[3] += bus_num;
		hw->strME = CPULD_pstrNewME(CPULD_enuME_TYPE_ISR,
						CPULD_enuME_STATE_NON_IDLE,
					    VID_IRQ_SPICC_PRIO + bus_num, name);
#endif
#ifdef HIFI
		xt_set_external_interrupt_handler(hw->irq,
						3, vSpiccHandleIsr, hw);
		xt_external_ints_on(hw->irq);
#elif defined ARCH_CPU_M4
		NVIC_vidIrqSetup(VID_IRQ_SPICC_BASE + bus_num,
			hw->irq, VID_IRQ_SPICC_PRIO + bus_num);
		NVIC_vidIrqEnable(VID_IRQ_SPICC_BASE + bus_num);
#elif defined CONFIG_RISCV
		RegisterIrq(hw->irq, 1, SpiccHandleIsrTable[bus_num]);
		EnableIrq(hw->irq);
#else
		hw->irq = 0;
		spi_err("interrupt unsupported\n");
#endif
	}

	xSpiccHwUnprepare(hw);
}

static int prvSpiccPrepare(struct SpiMaster *master, struct SpiDevice *spi)
{
	struct SpiccHw *hw;

	hw = pvSpiMasterGetDevData(master);
	return xSpiccHwPrepare(hw, spi->max_speed_hz, spi->mode,
			       spi->bits_per_word, spi->latency);
}

static int prvSpiccUnprepare(struct SpiMaster *master)
{
	struct SpiccHw *hw;

	hw = pvSpiMasterGetDevData(master);
	return xSpiccHwUnprepare(hw);
}

static int prvSpiccTransferOne(struct SpiMaster *master,
			       struct SpiDevice *spi,
			       struct SpiTransfer *t)
{
	struct SpiccHw *hw;
	int ret;

	hw = pvSpiMasterGetDevData(master);
	return xSpiccHwXfer(hw, t->tx_buf, t->rx_buf, t->len);
}

int xSpiccProbe(struct SpiccHwPlatformData *pdata)
{
	struct SpiMaster *master;
	struct SpiccHw *hw;
	int ret;

	master = pvSpiAllocMaster(sizeof(struct SpiccHw));
	if (!master)
		return -ENOMEM;

	hw = pvSpiMasterGetDevData(master);
	hw->base = pdata->reg;
	hw->irq = pdata->irq;
	hw->is_slave = pdata->is_slave;
	hw->force_ssctl = pdata->force_ssctl;
	hw->clk_rate = pdata->clk_rate;
	vSpiccHwInit(hw, pdata->compatible, pdata->bus_num);

	master->bus_num = pdata->bus_num;
	master->is_slave = hw->is_slave;
	master->cs_gpios = pdata->cs_gpios;
	master->num_chipselect = pdata->num_chipselect;
	master->prepare = prvSpiccPrepare;
	master->unprepare = prvSpiccUnprepare;
	master->transfer_one = prvSpiccTransferOne;

	ret = xSpiRegisterMaster(master);
	if (ret) {
		spi_err("Register spi controller failed\n");
		//spi_put_master(master);
		return ret;
	}
	spi_info("spi %s-controller%d@0x%x initialized(compatible=%d)\n",
		master->is_slave ? "slave" : "master",
		pdata->bus_num, (unsigned int)pdata->reg, pdata->compatible);

	return 0;
}
