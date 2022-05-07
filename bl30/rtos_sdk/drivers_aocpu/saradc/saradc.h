/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SARADC_H_
#define _SARADC_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <saradc-data.h>

/**
 * enum AdcChannelType - type of ADC Channel
 */
enum AdcChannelType {
	SARADC_CH0 = 0x0,
	SARADC_CH1,
	SARADC_CH2,
	SARADC_CH3,
	SARADC_CH4,
	SARADC_CH5,
	SARADC_CH6,
	SARADC_CH7,
	SARADC_CH_MAX,
};

/**
 * enum AdcAvgMode - mode of ADC averaging
 */
enum AdcAvgMode {
	NO_AVERAGING = 0x0,
	MEAN_AVERAGING = 0x1,
	MEDIAN_AVERAGING = 0x2,
};

/**
 * enum AdcNumSamples - number of samples to acquire 2^N
 */
enum AdcNumSamples {
	ONE_SAMPLE = 0x0,
	TWO_SAMPLES = 0x1,
	FOUR_SAMPLES = 0x2,
	EIGHT_SAMPLES = 0x3,
};

/**
 * struct AdcInstanceConfig - describe ADC configuration
 *
 * @channel: which channel to sample
 * @avgMode: averaging mode
 * @number:  number of samples each sampling, the maximum value is 16
 */
struct AdcInstanceConfig {
	enum AdcChannelType channel;
	enum AdcAvgMode avgMode;
	uint8_t number;
};

/**
 * vAdcInit() - initialize SARADC
 */
extern void vAdcInit(void);

/**
 * vAdcDeinit() - deinitialize SARADC
 */
extern void vAdcDeinit(void);

/**
 * vAdcHwEnable() - enable SARADC
 *
 * Note: can't be called from interrupt context
 */
extern void vAdcHwEnable(void);

/**
 * vAdcHwEnable() - disable SARADC
 *
 * Note: can't be called from interrupt context
 */
extern void vAdcHwDisable(void);

/**
 * xAdcGetSample() - obtain sampling value of SARADC
 * @data: sampling value save to
 * @datNum: number of elements in @data
 * @conf: instantiate a configuration
 *
 * return actual number of elements saved into @data on success,
 * negative value on failure
 *
 * Note: can't be called from interrupt context
 */
extern int32_t xAdcGetSample(uint16_t *data, uint16_t datNum, struct AdcInstanceConfig *conf);

/**
 * vAdcSelfTest() - SARADC self-test
 *
 * Note: can't be called from interrupt context
 */
extern void vAdcSelfTest(void);

#ifdef __cplusplus
}
#endif
#endif /* _SARADC_H_ */
