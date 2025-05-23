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
#include "gpio.h"
#include "task.h"
#include "spi.h"

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

#if CONFIG_SPI_TEST
static TaskHandle_t SpiTestTaskHandler;
#define SpiTestTaskPriority 4

void vSpicc1Test(void)
{
	struct SpiDevice *spi;
	struct SpiBoardInfo info = {
		.platform_data = NULL,
		.bus_num = 1,
		.chip_select = 0,
		.max_speed_hz = 10000000,
		.mode = SPI_MODE_3 | SPI_LOOP,
		.bits_per_word = 8,
		.latency = 0,
	};

	spi = pxSpiNewDevice(&info);
	if (!spi)
		return;

	xTaskCreate(vSpiTestTask, "SpiTest", configMINIMAL_STACK_SIZE, spi,
		    SpiTestTaskPriority, &SpiTestTaskHandler);
}
#endif