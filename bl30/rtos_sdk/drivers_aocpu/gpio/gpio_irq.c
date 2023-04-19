/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "register.h"
#include "projdefs.h"
#include "gpio_irq.h"
#include "semphr.h"

#include <unistd.h>
#include "n200_func.h"
#include "common.h"

#define DRIVER_NAME "gpio_irq"

static uint32_t GpioIrqRegBackup[IRQ_REG_NUM] = { 0 };

/* old platform like t5/t5d */
#ifdef GPIO_AO_IRQ_BASE
static uint32_t GpioIrqRegAOBackup;
#endif

/* platform a4 */
#ifdef GPIO_AO_EXT_IRQ_NUM
static uint32_t GpioIrqRegAOExtBackup[GPIO_AO_EXT_IRQ_NUM];
#endif

void vGpioIRQInit(void)
{
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE, BIT(31), BIT(31));
#ifdef GPIO_AO_EXT_IRQ_NUM
	REG32_UPDATE_BITS(GPIO_AO_IRQ_BASE, BIT(31), BIT(31));
#endif
}

/*
 * irqnum: gpio controller irqnum
 * line: gpio irq line
 */
static void prvGpioSetupIRQ(uint16_t irqNum, uint8_t line, uint32_t flags)
{
	prvGpioPlatIrqSetup(irqNum, line, flags);
}

static int32_t prvRequestParentIRQ(uint16_t gpio, GpioIRQHandler_t handler, uint32_t flags)
{
	const struct GpioIRQBank *bk = &(pGetGpioIrqBank()[gpio >> 5]);
	uint8_t offset = gpio % 32;
	uint16_t irq;
	uint8_t i;

	irq = bk->gpioIRQBase + offset;

	for (i = 0; i < bk->parentIRQNum; i++) {
		if (bk->parentIRQs[i].owner == gpio && (bk->parentIRQs[i].flags == flags)) {
			printf("%s: irq had been allocated for gpio[%d]\n", DRIVER_NAME, gpio);
			return -pdFREERTOS_ERRNO_EINVAL;
		}
		if (bk->parentIRQs[i].owner == GPIO_INVALID)
			break;
	}

	if (i == bk->parentIRQNum) {
		printf("%s: no more gpio irqs available for gpio[%d]\n", DRIVER_NAME, gpio);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	bk->parentIRQs[i].owner = gpio;
	bk->parentIRQs[i].flags = flags;

	prvGpioSetupIRQ(irq, i, flags);

	printf("bk->parentIRQs[i].irq is %d\n", bk->parentIRQs[i].irq);

	RegisterIrq(bk->parentIRQs[i].irq, 2, handler);
	ClearPendingIrq(bk->parentIRQs[i].irq);
	EnableIrq(bk->parentIRQs[i].irq);

	return 0;
}

static void prvFreeParentIRQ(uint16_t gpio)
{
	const struct GpioIRQBank *bk = &(pGetGpioIrqBank()[gpio >> 5]);
	uint8_t i;

	for (i = 0; i < bk->parentIRQNum; i++) {
		if (bk->parentIRQs[i].owner == gpio) {
			DisableIrq(bk->parentIRQs[i].irq);
			bk->parentIRQs[i].owner = GPIO_INVALID;
			bk->parentIRQs[i].flags = 0;
			UnRegisterIrq(bk->parentIRQs[i].irq);
		}
	}
}

int32_t xRequestGpioIRQ(uint16_t gpio, GpioIRQHandler_t handler, uint32_t flags)
{
	int32_t ret;

	ret = prvRequestParentIRQ(gpio, handler, flags);
	if (ret) {
		printf("%s: fail to allocate Parent irq for gpio[%d]\n", DRIVER_NAME, gpio);
		prvFreeParentIRQ(gpio);
		return ret;
	}

	return 0;
}

void vFreeGpioIRQ(uint16_t gpio)
{
	prvFreeParentIRQ(gpio);
}

void vEnableGpioIRQ(uint16_t gpio)
{
	const struct GpioIRQBank *bk = &(pGetGpioIrqBank()[gpio >> 5]);
	uint8_t i;

	for (i = 0; i < bk->parentIRQNum; i++) {
		if (bk->parentIRQs[i].owner == gpio) {
			ClearPendingIrq(bk->parentIRQs[i].irq);
			EnableIrq(bk->parentIRQs[i].irq);
		}
	}
}

void vDisableGpioIRQ(uint16_t gpio)
{
	const struct GpioIRQBank *bk = &(pGetGpioIrqBank()[gpio >> 5]);
	uint8_t i;

	for (i = 0; i < bk->parentIRQNum; i++) {
		if (bk->parentIRQs[i].owner == gpio)
			DisableIrq(bk->parentIRQs[i].irq);
	}
}

/* resume */
void vRestoreGpioIrqReg(void)
{
	uint8_t ucIndex;

	for (ucIndex = 0; ucIndex < IRQ_REG_NUM; ucIndex++)
		REG32((unsigned long)(GPIO_EE_IRQ_BASE + 0x04 * ucIndex)) =
			GpioIrqRegBackup[ucIndex];

/* platform a4 */
#ifdef GPIO_AO_EXT_IRQ_NUM
	for (ucIndex = 0; ucIndex < GPIO_AO_EXT_IRQ_NUM; ucIndex++)
		REG32((unsigned long)(GPIO_AO_IRQ_BASE + 0x04 * ucIndex)) =
			GpioIrqRegAOExtBackup[ucIndex];
#else
/* old platform like t5/t5d */
#ifdef GPIO_AO_IRQ_BASE
	REG32((unsigned long)GPIO_AO_IRQ_BASE) = GpioIrqRegAOBackup;
#endif
#endif
}

/* when come into suspend before request gpio irq*/
void vBackupAndClearGpioIrqReg(void)
{
	uint8_t ucIndex;

	for (ucIndex = 0; ucIndex < IRQ_REG_NUM; ucIndex++) {
		GpioIrqRegBackup[ucIndex] = REG32((unsigned long)GPIO_EE_IRQ_BASE + 0x04 * ucIndex);
		//printf("reg[%d] is 0x%x\n", ucIndex, GpioIrqRegBackup[ucIndex]);
	}

	for (ucIndex = 0; ucIndex < IRQ_REG_NUM; ucIndex++)
		REG32((unsigned long)(GPIO_EE_IRQ_BASE + 0x04 * ucIndex)) = 0;

/* platform a4 */
#ifdef GPIO_AO_EXT_IRQ_NUM
	for (ucIndex = 0; ucIndex < GPIO_AO_EXT_IRQ_NUM; ucIndex++)
		GpioIrqRegAOExtBackup[ucIndex] =
			REG32((unsigned long)GPIO_AO_IRQ_BASE + 0x04 * ucIndex);

	for (ucIndex = 0; ucIndex < GPIO_AO_EXT_IRQ_NUM; ucIndex++)
		REG32((unsigned long)(GPIO_AO_IRQ_BASE + 0x04 * ucIndex)) = 0;
#else
/* old platform like t5/t5d */
#ifdef GPIO_AO_IRQ_BASE
	GpioIrqRegAOBackup = REG32((unsigned long)GPIO_AO_IRQ_BASE);
	REG32((unsigned long)GPIO_AO_IRQ_BASE) = 0;
#endif
#endif
}
