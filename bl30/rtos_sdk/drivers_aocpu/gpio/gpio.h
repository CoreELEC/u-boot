/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <gpio-data.h>

/* trigger type for GPIO IRQ */
#define IRQF_TRIGGER_NONE 0x00000000
#define IRQF_TRIGGER_RISING 0x00000001
#define IRQF_TRIGGER_FALLING 0x00000002
#define IRQF_TRIGGER_HIGH 0x00000004
#define IRQF_TRIGGER_LOW 0x00000008
#define IRQF_TRIGGER_BOTH 0x00000010

/* pin features */
#define PINF_CONFIG_BIAS_DISABLE 0x00000001
#define PINF_CONFIG_BIAS_PULL_UP 0x00000002
#define PINF_CONFIG_BIAS_PULL_DOWN 0x00000004
#define PINF_CONFIG_DRV_STRENGTH_0 0x00000008
#define PINF_CONFIG_DRV_STRENGTH_1 0x00000010
#define PINF_CONFIG_DRV_STRENGTH_2 0x00000020
#define PINF_CONFIG_DRV_STRENGTH_3 0x00000040

#define PINF_CONFIG_DRV_MASK 0x00000078

/**
 * enum GpioDirtype - type of gpio direction
 */
enum GpioDirType {
	GPIO_DIR_OUT = 0x0,
	GPIO_DIR_IN,
	GPIO_DIR_INVALID,
};

/**
 * enum GpioOutLevelType - type of gpio output level
 */
enum GpioOutLevelType {
	GPIO_LEVEL_LOW = 0x0,
	GPIO_LEVEL_HIGH,
	GPIO_LEVEL_INVALID,
};

/**
 * enum PinMuxType - type of pin mux
 */
enum PinMuxType {
	PIN_FUNC0 = 0x0,
	PIN_FUNC1,
	PIN_FUNC2,
	PIN_FUNC3,
	PIN_FUNC4,
	PIN_FUNC5,
	PIN_FUNC6,
	PIN_FUNC7,
	PIN_FUNC_INVALID,
};

typedef void (*GpioIRQHandler_t)(void);

/**
 * xGpioSetDir() - Set gpio direction
 * @gpio: GPIO number
 * @dir : direction value, and defined in "enum GpioDirType"
 *
 * Returns 0 on success, negative value on error.
 */
extern int xGpioSetDir(uint16_t gpio, enum GpioDirType dir);

/**
 * xGpioSetValue() - Configure output level on GPIO pin
 * @gpio  : GPIO number
 * @level : level value, and defined in "enum GpioOutLevelType"
 *
 * Returns 0 on success, negative value on error.
 */
extern int xGpioSetValue(uint16_t gpio, enum GpioOutLevelType level);

/**
 * xGpioGetValue() - Sample GPIO pin and return it's value
 * @gpio: GPIO number
 *
 */
extern int xGpioGetValue(uint16_t gpio);

/**
 * xPinconfSet() - Configure pin features
 * @gpio  : GPIO number
 * @flags : pin features
 *
 * Returns 0 on success, negative value on error.
 */
extern int xPinconfSet(uint16_t gpio, uint32_t flags);

/**
 * xPinmuxSet() - Select function for per pin
 * @gpio : GPIO number
 * @func : function value, and defined in "enum PinMuxType"
 *
 * Returns 0 on success, negative value on error.
 */
extern int xPinmuxSet(uint16_t gpio, enum PinMuxType func);

/**
 * vGpioIRQInit() - initialize gpio IRQ
 *
 */
extern void vGpioIRQInit(void);

/**
 * xRequestGpioIRQ() - Request IRQ for gpio
 * @gpio   : GPIO number
 * @handler: interrupt handler function
 * @flags  : Trigger type
 *
 * Returns 0 on success, negative value on error.
 *
 * Note: can't be called from interrupt context
 */
extern int32_t xRequestGpioIRQ(uint16_t gpio, GpioIRQHandler_t handler, uint32_t flags);

/**
 * vFreeGpioIRQ() - Free IRQ for gpio
 * @gpio   : GPIO number
 *
 * Note: can't be called from interrupt context
 */
extern void vFreeGpioIRQ(uint16_t gpio);

/**
 * vEnableGpioIRQ() - Enable IRQ for gpio
 * @gpio   : GPIO number
 *
 */
extern void vEnableGpioIRQ(uint16_t gpio);

/**
 * vDisableGpioIRQ() - Disable IRQ for gpio
 * @gpio   : GPIO number
 *
 */
extern void vDisableGpioIRQ(uint16_t gpio);

/**
 * restore and backup irqreg
 *
 */
extern void vRestoreGpioIrqReg(void);
extern void vBackupAndClearGpioIrqReg(void);

/**
 * restore and backup gpio register
 * @name : Bank name
 *
 * Returns 0 on success, negative value on error.
 */
	extern int xBankStateBackup(const char *name);
	extern int xBankStateRestore(const char *name);

#ifdef __cplusplus
}
#endif
#endif /* _GPIO_H_ */
