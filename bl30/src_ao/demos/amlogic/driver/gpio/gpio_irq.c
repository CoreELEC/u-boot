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
 * gpio driver
 */
#include "FreeRTOS.h"
#include "register.h"
#include "util.h"
#include "projdefs.h"
#include "gpio_irq.h"
#include "semphr.h"

#include <unistd.h>
#include "n200_eclic.h"
#include "n200_func.h"
#include "common.h"

#define  DRIVER_NAME  "gpio_irq"

struct vGpioHandler {
	void (*vhandler)(void);
	uint8_t ucIrqNum;
};

static struct vGpioHandler vGpioHandlerGroup[] = {
	{NULL, IRQ_NUM_GPIO0},
	{NULL, IRQ_NUM_GPIO1},
	{NULL, IRQ_NUM_GPIO2},
	{NULL, IRQ_NUM_GPIO3},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
};


static uint32_t GpioIrqRegBackup[IRQ_REG_NUM] = {0};

void vGpioIRQInit(void)
{
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE, BIT(31), BIT(31));
}

/* irqnum: gpio controller irqnum
 * line: gpio irq line
 * */
static void prvGpioSetupIRQ(uint16_t irqNum, uint8_t line, uint32_t flags)
{
	uint32_t val = 0;
	uint32_t reg_offset = 0;
	uint16_t bit_offset = 0;

	bit_offset = ((line % 2) == 0) ? 0 : 16;
	reg_offset = REG_PIN_SC2_SEL + ((line / 2) << 2);

	/* clear both edge */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_EDGE_POL_EXTR,
			  BIT(0 + (line)), 0);

	/* set filter */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + reg_offset,
			  0x7 << GPIO_IRQ_FILTER_SHIFT(line),
			  0x7 << GPIO_IRQ_FILTER_SHIFT(line));

	/* select trigger pin */
	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + reg_offset,
			  0x7f << bit_offset, irqNum << bit_offset);

	/* set trigger both type */
	if (flags & IRQF_TRIGGER_BOTH) {
		val |= BIT(0 + (line));
		REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE + REG_EDGE_POL_EXTR,
				  BIT(0 + (line)), val);
		return;
	}

	/* set trigger single edge or level  type */
	if (flags & (IRQF_TRIGGER_LOW | IRQF_TRIGGER_FALLING))
		val |= BIT(0 + (line));

	if (flags & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING))
		val |= BIT(12 + (line));

	REG32_UPDATE_BITS(GPIO_EE_IRQ_BASE, REG_EDGE_POL_MASK_SC2(line), val);

	return;

}

static int32_t prvRequestParentIRQ(uint16_t gpio, GpioIRQHandler_t handler,
				   uint32_t flags)
{
	const GpioIRQBank_t *bk = &(pGetGpioIrqBank()[gpio >> 5]);
	uint8_t offset = gpio % 32;
	uint16_t irq;
	uint8_t i;
	uint8_t ucCurIrqNum;

	irq = bk->gpioIRQBase + offset;

	for (i = 0; i < bk->parentIRQNum; i++) {
		if (bk->parentIRQs[i].owner == gpio &&
		    (bk->parentIRQs[i].flags == flags)) {
			iprintf("%s: irq had been allocated for gpio[%d]\n",
				DRIVER_NAME, gpio);
			return -pdFREERTOS_ERRNO_EINVAL;
		}
		if (bk->parentIRQs[i].owner == GPIO_INVALID)
			break;
	}

	if (i == bk->parentIRQNum) {
		iprintf("%s: no more gpio irqs available for gpio[%d]\n",
			DRIVER_NAME, gpio);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	bk->parentIRQs[i].owner = gpio;
	bk->parentIRQs[i].flags = flags;

	prvGpioSetupIRQ(irq, i, flags);

	ucCurIrqNum = vGpioHandlerGroup[i].ucIrqNum;
	iprintf("bk->parentIRQs[i].irq is %d\n", bk->parentIRQs[i].irq);
	if (ucCurIrqNum) {
		vGpioHandlerGroup[i].vhandler = handler;
		ClearPendingIrq(ucCurIrqNum);
		printf("%s: TODO: please use new vEnableIiq function.\n", __func__);
		//vEnableIrq(ucCurIrqNum, bk->parentIRQs[i].irq);
	}

	return 0;
}

static void prvFreeParentIRQ(uint16_t gpio)
{
	const GpioIRQBank_t *bk = &(pGetGpioIrqBank()[gpio >> 5]);
	uint8_t i;

	for (i = 0; i < bk->parentIRQNum; i++) {
		if (bk->parentIRQs[i].owner == gpio) {
			DisableIrq(vGpioHandlerGroup[i].ucIrqNum);
			vGpioHandlerGroup[i].vhandler = NULL;
			bk->parentIRQs[i].owner = GPIO_INVALID;
			bk->parentIRQs[i].flags = 0;
		}
	}
}

int32_t xRequestGpioIRQ(uint16_t gpio, GpioIRQHandler_t handler, uint32_t flags)
{
	int32_t ret;

	ret = prvRequestParentIRQ(gpio, handler, flags);
	if (ret) {
		iprintf("%s: fail to allocate Parent irq for gpio[%d]\n",
			DRIVER_NAME, gpio);
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
	const GpioIRQBank_t *bk = &(pGetGpioIrqBank()[gpio >> 5]);
	uint8_t i;

	for (i = 0; i < bk->parentIRQNum; i++) {
		if (bk->parentIRQs[i].owner == gpio) {
			ClearPendingIrq(vGpioHandlerGroup[i].ucIrqNum);
		printf("%s: TODO: please use new vEnableIiq function.\n", __func__);
			//vEnableIrq(vGpioHandlerGroup[i].ucIrqNum,
			//	   bk->parentIRQs[i].irq);
		}
	}
}

void vDisableGpioIRQ(uint16_t gpio)
{
	const GpioIRQBank_t *bk = &(pGetGpioIrqBank()[gpio >> 5]);
	uint8_t i;

	for (i = 0; i < bk->parentIRQNum; i++) {
		if (bk->parentIRQs[i].owner == gpio) {
			DisableIrq(vGpioHandlerGroup[i].ucIrqNum);
		}
	}
}

/* resume */
void vRestoreGpioIrqReg(void)
{
	uint8_t ucIndex;
	for (ucIndex = 0; ucIndex < IRQ_REG_NUM; ucIndex++)
		REG32((unsigned long)(GPIO_EE_IRQ_BASE + 0x04 * ucIndex))
		= GpioIrqRegBackup[ucIndex];
}

/* when come into suspend before request gpio irq*/
void vBackupAndClearGpioIrqReg(void)
{
	uint8_t ucIndex;
	for (ucIndex = 0; ucIndex < IRQ_REG_NUM; ucIndex++) {
		GpioIrqRegBackup[ucIndex] =
			REG32((unsigned long)GPIO_EE_IRQ_BASE + 0x04 * ucIndex);
		//iprintf("reg[%d] is 0x%x\n", ucIndex, GpioIrqRegBackup[ucIndex]);
	}

	for (ucIndex = 0; ucIndex < IRQ_REG_NUM; ucIndex++)
		REG32((unsigned long)(GPIO_EE_IRQ_BASE + 0x04 * ucIndex)) = 0;

}

/* add more if necessary */
static void GPIO_IRQ_Handler0(void)
{
	if (vGpioHandlerGroup[0].vhandler)
		vGpioHandlerGroup[0].vhandler();
}

static void GPIO_IRQ_Handler1(void)
{
	if (vGpioHandlerGroup[1].vhandler)
		vGpioHandlerGroup[1].vhandler();
}

static void GPIO_IRQ_Handler2(void)
{
	if (vGpioHandlerGroup[2].vhandler)
		vGpioHandlerGroup[2].vhandler();
}

static void GPIO_IRQ_Handler3(void)
{
	if (vGpioHandlerGroup[3].vhandler)
		vGpioHandlerGroup[3].vhandler();
}

DECLARE_IRQ(IRQ_NUM_GPIO0, GPIO_IRQ_Handler0)
DECLARE_IRQ(IRQ_NUM_GPIO1, GPIO_IRQ_Handler1)
DECLARE_IRQ(IRQ_NUM_GPIO2, GPIO_IRQ_Handler2)
DECLARE_IRQ(IRQ_NUM_GPIO3, GPIO_IRQ_Handler3)
