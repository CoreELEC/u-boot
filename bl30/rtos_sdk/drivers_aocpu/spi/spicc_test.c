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
#include "task.h"
#include "gpio.h"
#include "spi.h"

static int xSpiMemCmp(void *src1, void *src2, int len)
{
	u8 *d1 = (u8 *)src1;
	u8 *d2 = (u8 *)src2;
	int i, diff = 0;

	if (!d1 || !d2) {
		spi_dbg("\tnull pointer, total %d\n", len);
		return 0;
	}

	for (i = 0; i < len; i++) {
		if (*d1 != *d2) {
			spi_dbg("\t%d: 0x%x, 0x%x\n", i, *d1, *d2);
			diff++;
		}
		d1++;
		d2++;
	}

	return diff;
}

static void vSpiMemSet(void *dest, u8 val, int len, u8 step)
{
	u8 *d = (u8 *)dest;
	int i;

	for (i = 0; i < len; i++) {
		*d++ = val & 0xff;
		val += step;
	}
}

#define SPICC_CTS_FCLK_DIV2	4
#define SPICC_CTS_DIV_5		4
#define SPICC1_CTS_DIV_MASK	GENMASK(21, 16)
#define SPICC1_CTS_GATE		BIT(22)
#define SPICC1_CTS_SEL_MASK	GENMASK(25, 23)

static const int Spicc1CsGpios[] = {GPIOH_22};

void vSpicc1Init(void)
{
	uint32_t val;

	struct SpiccHwPlatformData pdata = {
		.compatible = MESON_T5M_SPICC,
		.reg = 0xfe052000,
		.is_slave = 0,
		.force_ssctl = 0,
		.irq = 184,
		.bus_num = 1,
		.cs_gpios = Spicc1CsGpios,
		.num_chipselect = ARRAY_SIZE(Spicc1CsGpios),
		.clk_rate = 200000000,
	};

	/* set spicc1 pinmux */
	xPinmuxSet(GPIOH_23, PIN_FUNC5);
	xPinmuxSet(GPIOH_24, PIN_FUNC5);
	xPinmuxSet(GPIOH_25, PIN_FUNC5);

	/* set spicc1 clktree */
	val = REG32(CLKCTRL_SPICC_CLK_CTRL) & ~(0x3ff << 16);
	val |= FIELD_PREP(SPICC1_CTS_SEL_MASK, SPICC_CTS_FCLK_DIV2)
	       | FIELD_PREP(SPICC1_CTS_DIV_MASK, SPICC_CTS_DIV_5)
	       | SPICC1_CTS_GATE;
	REG32(CLKCTRL_SPICC_CLK_CTRL) = val;

	xSpiccProbe(&pdata);
}

void vSpiMasterTask(void *pvParameter)
{
	struct SpiDevice *spi;
	struct SpiMessage msg;
	struct SpiTransfer *t;
	int count = 0, i;
	int ret;

	struct SpiBoardInfo info = {
		.platform_data = NULL,
		.bus_num = 1,
		.chip_select = 0,
		.max_speed_hz = 10000000,
		.mode = SPI_MODE_3 | SPI_LOOP,
		.bits_per_word = 8,
		.latency = 0,
	};

	struct SpiTransfer xfers[] = {
		{
		    .len = 64,
		    .delay_usecs = 0,
		    .cs_change = 0,
		},
	};

	spi = pxSpiNewDevice(&info);
	if (!spi)
		return;

	for (i = 0; i < ARRAY_SIZE(xfers); i++) {
		t = &xfers[i];
		t->tx_buf = pvPortMallocAlign(t->len, 0xF);
		t->rx_buf = pvPortMallocAlign(t->len, 0xF);
		vSpiMemSet((void *)t->tx_buf, 1, t->len, i + 1);
	}

	vSpiMessageInit(&msg, xfers, ARRAY_SIZE(xfers));
	while (++count <= 5) {
		int diff;

		vTaskDelay(pdMS_TO_TICKS(1000));
		ret = xSpiSync(spi, &msg);
		printf("\n%dth test %s, time %d usec\n", count,
		       ret ? "failed" : "success", msg.time_consump);
		if (ret)
			continue;

		for (i = 0; i < msg.num_xfers; i++) {
			t = &msg.xfers[i];
			diff = xSpiMemCmp((void *)t->tx_buf, (void *)t->rx_buf, t->len);
			printf("xfer[%d]: total %d, diff %d, time %d\n",
			       i, t->len, diff, t->time_consump);
			memset(t->rx_buf, 0, t->len);
		}
	}

	vSpiUnregisterDevice(spi);
	for (i = 0; i < ARRAY_SIZE(xfers); i++) {
		t = &xfers[i];
		vPortFree((void *)t->tx_buf);
		vPortFree(t->rx_buf);
	}

	vTaskDelete(NULL);
}



