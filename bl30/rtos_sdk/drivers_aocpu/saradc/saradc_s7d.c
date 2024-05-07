/*
 * Copyright (c) 2021-2024 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "projdefs.h"
#include "portmacro.h"
#include "semphr.h"
#include "task.h"
#include "saradc.h"

#include <stdbool.h>
#include <unistd.h>
#include "n200_func.h"
#include "common.h"

#define SARADC_DRV_NAME				"saradc"

#define SARADC_MAX_FIFO_SIZE			16

#define SAR_CLK_DIV_MASK			GENMASK(7, 0)
#define SAR_CLK_DIV_SHIFT			(0)
#define SAR_CLK_GATE				BIT(8)
#define SAR_CLK_MUX_MASK			GENMASK(10, 9)

#define P_SARADC(x)				(SARADC_BASE + (x))

#define SARADC_REG0				0x00
#define SARADC_REG0_SAMPLING_STOP		BIT(14)
#define SARADC_REG0_ADC_EN			BIT(9)
#define SARADC_REG0_FIFO_CNT_IRQ_MASK		GENMASK(8, 4)
#define SARADC_REG0_FIFO_CNT_IRQ_SHIFT		(4)
#define SARADC_REG0_FIFO_IRQ_EN			BIT(3)
#define SARADC_REG0_SAMPLE_START		BIT(2)
#define SARADC_REG0_CONTINUOUS_EN		BIT(1)
#define SARADC_REG0_SAMPLING_ENABLE		BIT(0)

#define SARADC_REG1				0x04
#define SARADC_REG1_MAX_INDEX_MASK		GENMASK(26, 24)
#define SARADC_REG1_ENTRY_SHIFT(_chan)		((_chan) * 3)
#define SARADC_REG1_ENTRY_MASK(_chan)		(GENMASK(2, 0) << ((_chan) * 3))

#define SARADC_REG2				0x08
#define SARADC_REG2_AVG_MODE_SHIFT(_chan)	(16 + ((_chan) * 2))
#define SARADC_REG2_AVG_MODE_MASK(_chan)	(GENMASK(17, 16) << ((_chan) * 2))
#define SARADC_REG2_NUM_SAMPLES_SHIFT(_chan)	(0 + ((_chan) * 2))
#define SARADC_REG2_NUM_SAMPLES_MASK(_chan)	(GENMASK(1, 0) << ((_chan) * 2))

#define SARADC_REG3				0x0c
#define SARADC_REG4				0x10
#define SARADC_REG5				0x14
#define SARADC_REG6				0x18
#define SARADC_REG7				0x1c
#define SARADC_REG8				0x20
#define SARADC_REG9				0x24
#define SARADC_REG10				0x28

#define SARADC_REG11				0x2c
#define SARADC_REG11_CHANNEL7_AUX_CTRL_MASK	GENMASK(27, 25)
#define SARADC_REG11_CHANNEL7_AUX_CTRL_SHIFT	(25)

#define SARADC_REG12				0x30
#define SARADC_REG13				0x34
#define SARADC_REG14				0x38

#define SARADC_STATUS0				0x3c
#define SARADC_STATUS0_BUSY_MASK		GENMASK(14, 12)
#define SARADC_STATUS0_FIFO_COUNT_MASK		GENMASK(7, 3)

#define SARADC_STATUS1				0x40
#define SARADC_STATUS2				0x44

#define SARADC_STATUS3				0x48
#define SARADC_STATUS3_FIFO_RD_CHAN_ID_SHIFT	(20)
#define SARADC_STATUS3_FIFO_RD_CHAN_ID_MASK	GENMASK(22, 20)

#define SARADC_STATUS4				0x4c
#define SARADC_STATUS5				0x50
#define SARADC_STATUS6				0x54
#define SARADC_STATUS7				0x58
#define SARADC_STATUS8				0x5c
#define SARADC_STATUS9				0x60
#define SARADC_STATUS10				0x64
#define SARADC_STATUS11				0x68
#define SARADC_STATUS12				0x6c
#define SARADC_STATUS13				0x70
#define SARADC_STATUS14				0x74
#define SARADC_STATUS15				0x78
#define SARADC_RDY				0x80

static void vAdcHandlerISR(void);

static uint32_t SaradcRegBackup[SARADC_REG_NUM] = { 0 };

const char *const ch7Vol[] = {
	"gnd", "vdd/4", "vdd/2", "vdd*3/4", "vdd",
};

static SemaphoreHandle_t adcSemaphoreMutex;
static SemaphoreHandle_t adcSemaphoreBinary;

static int calibParamK;
static int calibParamB;
static bool calibValid;
static int32_t xAdcDoCalibration(void);

static void vBackupSaradcReg(void)
{
	uint8_t ucIndex;

	for (ucIndex = 0; ucIndex < (SARADC_REG_NUM - 1); ucIndex++) {
		SaradcRegBackup[ucIndex] =
			REG32((unsigned long)P_SARADC(SARADC_REG0) + 0x04 * ucIndex);
	}

	/* saradc clock reg */
	SaradcRegBackup[SARADC_REG_NUM - 1] = REG32((unsigned long)SAR_CLK_BASE);
}

static void vRestoreSaradcReg(void)
{
	uint8_t ucIndex;

	for (ucIndex = 0; ucIndex < (SARADC_REG_NUM - 1); ucIndex++) {
		REG32((unsigned long)(P_SARADC(SARADC_REG0) + 0x04 * ucIndex)) =
			SaradcRegBackup[ucIndex];
	}

	/* saradc clock reg */
	REG32((unsigned long)SAR_CLK_BASE) = SaradcRegBackup[SARADC_REG_NUM - 1];
}

void vAdcInit(void)
{
	vBackupSaradcReg();

	/* Filter control */
	REG32(P_SARADC(SARADC_REG7)) = 0x00000c21;
	REG32(P_SARADC(SARADC_REG8)) = 0x0280614d;

	/* Delay configure */
	REG32(P_SARADC(SARADC_REG3)) = 0x10a02403;
	REG32(P_SARADC(SARADC_REG4)) = 0x00000080;
	REG32(P_SARADC(SARADC_REG5)) = 0x0010340b;

	/* Control */
	REG32(P_SARADC(SARADC_REG6)) = 0x00000031;

	/* Set aux and extern vref */
	REG32(P_SARADC(SARADC_REG9)) = 0x0000e4e4;
	REG32(P_SARADC(SARADC_REG10)) = 0x74543414;
	REG32(P_SARADC(SARADC_REG11)) = 0xf4d4b494;

	/* Configure the averaging mode of the channels we use */
	REG32(P_SARADC(SARADC_REG2)) = (0x00000000);

	/* ADC is disabled by default */
	REG32(P_SARADC(SARADC_REG0)) = (0x00400000);

	/* Clock initialization: 3M=24M/(0x7 + 1) */
	REG32_UPDATE_BITS(SAR_CLK_BASE, SAR_CLK_DIV_MASK, 0x7 << SAR_CLK_DIV_SHIFT);

	/* Interrupt initialization */
	RegisterIrq(SARADC_INTERRUPT_NUM, 1, vAdcHandlerISR);
	EnableIrq(SARADC_INTERRUPT_NUM);

	/* Create mutex semaphore */
	adcSemaphoreMutex = xSemaphoreCreateMutex();
	configASSERT(adcSemaphoreMutex != NULL);

	/* Create binary semaphore */
	adcSemaphoreBinary = xSemaphoreCreateBinary();
	configASSERT(adcSemaphoreBinary != NULL);

	/* Get the parameters required for calibration */
	xAdcDoCalibration();
}

void vAdcDeinit(void)
{
	DisableIrq(SARADC_INTERRUPT_NUM);
	UnRegisterIrq(SARADC_INTERRUPT_NUM);

	vSemaphoreDelete(adcSemaphoreMutex);
	vSemaphoreDelete(adcSemaphoreBinary);

	vRestoreSaradcReg();
}

void vAdcHwEnable(void)
{
	xSemaphoreTake(adcSemaphoreMutex, portMAX_DELAY);

	REG32_UPDATE_BITS(SAR_CLK_BASE, SAR_CLK_GATE, SAR_CLK_GATE);

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0), SARADC_REG0_ADC_EN,
			  SARADC_REG0_ADC_EN);

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0), SARADC_REG0_SAMPLING_ENABLE,
			  SARADC_REG0_SAMPLING_ENABLE);

	xSemaphoreGive(adcSemaphoreMutex);
}

void vAdcHwDisable(void)
{
	xSemaphoreTake(adcSemaphoreMutex, portMAX_DELAY);

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0), SARADC_REG0_SAMPLING_ENABLE, 0);

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0), SARADC_REG0_ADC_EN, 0);

	REG32_UPDATE_BITS(SAR_CLK_BASE, SAR_CLK_GATE, 0);

	xSemaphoreGive(adcSemaphoreMutex);
}

static inline void prvAdcClearFifo(void)
{
	uint16_t i;

	for (i = 0; i < 32; i++) {
		if (!(REG32(P_SARADC(SARADC_STATUS0)) & SARADC_STATUS0_FIFO_COUNT_MASK))
			break;

		REG32(P_SARADC(SARADC_STATUS3));
	}
}

static inline void prvAdcEnableChannel(enum AdcChannelType ch, uint8_t idx)
{
	REG32_UPDATE_BITS(P_SARADC(SARADC_REG1), SARADC_REG1_ENTRY_MASK(idx),
			  ch << SARADC_REG1_ENTRY_SHIFT(idx));
}

static inline void prvAdcStartSample(void)
{
	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0),
			  SARADC_REG0_FIFO_IRQ_EN | SARADC_REG0_SAMPLING_STOP,
			  SARADC_REG0_FIFO_IRQ_EN);

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0), SARADC_REG0_SAMPLE_START,
			  SARADC_REG0_SAMPLE_START);
}

static inline void prvAdcStopSample(void)
{
	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0), SARADC_REG0_SAMPLE_START, 0);

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0),
			  SARADC_REG0_SAMPLING_STOP | SARADC_REG0_FIFO_IRQ_EN,
			  SARADC_REG0_SAMPLING_STOP);
}

static inline void prvAdcSetAvgMode(enum AdcChannelType ch, enum AdcAvgMode mode)
{
	if (mode == MEDIAN_AVERAGING)
		REG32_UPDATE_BITS(P_SARADC(SARADC_REG2), SARADC_REG2_NUM_SAMPLES_MASK(ch),
				  EIGHT_SAMPLES << SARADC_REG2_NUM_SAMPLES_SHIFT(ch));
	else if (mode == MEAN_AVERAGING)
		REG32_UPDATE_BITS(P_SARADC(SARADC_REG2), SARADC_REG2_NUM_SAMPLES_MASK(ch),
				  FOUR_SAMPLES << SARADC_REG2_NUM_SAMPLES_SHIFT(ch));
	else if (mode == NO_AVERAGING)
		REG32_UPDATE_BITS(P_SARADC(SARADC_REG2), SARADC_REG2_NUM_SAMPLES_MASK(ch),
				  NO_AVERAGING << SARADC_REG2_NUM_SAMPLES_SHIFT(ch));

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG2), SARADC_REG2_AVG_MODE_MASK(ch),
			  mode << SARADC_REG2_AVG_MODE_SHIFT(ch));
}

static inline void prvAdcUpdateIntThresh(uint8_t thresh)
{
	configASSERT(thresh <= SARADC_MAX_FIFO_SIZE);

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0), SARADC_REG0_CONTINUOUS_EN,
			  ((thresh == 1) ? 0 : SARADC_REG0_CONTINUOUS_EN));

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG0), SARADC_REG0_FIFO_CNT_IRQ_MASK,
			  thresh << SARADC_REG0_FIFO_CNT_IRQ_SHIFT);
}

static int32_t prvAdcReadRawSample(uint16_t *data, uint16_t datNum, struct AdcInstanceConfig *conf)
{
	uint8_t fifo_ch;
	uint16_t count = 0;
	uint32_t rVal;
	int tmp;

	if (xSemaphoreTake(adcSemaphoreBinary, 100 / portTICK_PERIOD_MS) == pdTRUE) {
		/* timeout 100ms */
		while (REG32(P_SARADC(SARADC_STATUS0)) & SARADC_STATUS0_FIFO_COUNT_MASK) {
			if (count >= conf->number || count >= datNum ||
			    count >= SARADC_MAX_FIFO_SIZE)
				break;

			rVal = REG32(P_SARADC(SARADC_STATUS3));

			fifo_ch = (rVal & SARADC_STATUS3_FIFO_RD_CHAN_ID_MASK) >>
				   SARADC_STATUS3_FIFO_RD_CHAN_ID_SHIFT;

			if (fifo_ch != conf->channel)
				return -pdFREERTOS_ERRNO_EINVAL;

			data[count] = rVal & 0xffff;

			if (calibValid) {
				tmp = (int)(short)data[count] - calibParamB;
				tmp = tmp < 0 ? 0 : tmp;
				tmp = tmp * 1000 / calibParamK;
				tmp = tmp > 0xfff ? 0xfff : tmp;
				data[count] = (uint16_t)tmp;
			}

			count++;
		}
	} else {
		prvAdcStopSample();
		return -pdFREERTOS_ERRNO_ETIMEDOUT;
	}

	return count;
}

static void vAdcHandlerISR(void)
{
	BaseType_t reschedule = pdFALSE;

	/* stop sampling before reading the data */
	prvAdcStopSample();

	xSemaphoreGiveFromISR(adcSemaphoreBinary, &reschedule);

	portYIELD_FROM_ISR(reschedule);
}

int32_t xAdcGetSample(uint16_t *data, uint16_t datNum, struct AdcInstanceConfig *conf)
{
	int32_t ret;

	xSemaphoreTake(adcSemaphoreMutex, portMAX_DELAY);

	prvAdcClearFifo();

	prvAdcEnableChannel(conf->channel, 0);

	prvAdcSetAvgMode(conf->channel, conf->avgMode);

	prvAdcUpdateIntThresh(conf->number);

	prvAdcStartSample();

	ret = prvAdcReadRawSample(data, datNum, conf);

	xSemaphoreGive(adcSemaphoreMutex);

	if (ret <= 0) {
		printf("%s: failed to read sample for channel %d: %d\n", SARADC_DRV_NAME,
		       conf->channel, ret);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	return ret;
}

static int32_t xAdcDoCalibration(void)
{
	int32_t ret;
	uint16_t raw0v;
	uint16_t raw1v8;
	struct AdcInstanceConfig adcConfig = { SARADC_CH7, MEAN_AVERAGING, 1 };

	if (calibValid)
		return 0;

	vAdcHwEnable();

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG11), SARADC_REG11_CHANNEL7_AUX_CTRL_MASK,
			  0 << SARADC_REG11_CHANNEL7_AUX_CTRL_SHIFT);
	ret = xAdcGetSample(&raw0v, 1, &adcConfig);
	if (ret <= 0)
		goto err;

	REG32_UPDATE_BITS(P_SARADC(SARADC_REG11), SARADC_REG11_CHANNEL7_AUX_CTRL_MASK,
			  4 << SARADC_REG11_CHANNEL7_AUX_CTRL_SHIFT);
	ret = xAdcGetSample(&raw1v8, 1, &adcConfig);
	if (ret <= 0)
		goto err;

	calibParamB = (short)raw0v;
	calibParamK = (((short)raw1v8 - (short)raw0v) * 1000) / 4095;
	if (calibParamK <= 0)
		goto err;
	calibValid = true;

	vAdcHwDisable();
	return 0;
err:
	printf("%s: An exception occurred during calibration\n", SARADC_DRV_NAME);
	vAdcHwDisable();
	return -pdFREERTOS_ERRNO_EINVAL;
}

void vAdcSelfTest(void)
{
	int32_t ret;
	uint32_t i;
	uint16_t adcData[1];
	struct AdcInstanceConfig adcConfig = { SARADC_CH7, NO_AVERAGING, 1 };

	vAdcHwEnable();

	for (i = 0; i < ARRAY_SIZE(ch7Vol); i++) {
		REG32_UPDATE_BITS(P_SARADC(SARADC_REG11), SARADC_REG11_CHANNEL7_AUX_CTRL_MASK,
				  i << SARADC_REG11_CHANNEL7_AUX_CTRL_SHIFT);

		ret = xAdcGetSample(adcData, 1, &adcConfig);
		if (ret <= 0)
			return;

		printf("%-8s: %d\n", ch7Vol[i], adcData[0]);
	}

	vAdcHwDisable();
}
