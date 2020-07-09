#include <stdlib.h>
#include <stdint.h>
#include "myprintf.h"

#include "mailbox-irq.h"
#include "mailbox.h"

#define TAG "AOCPU"
#define PRINT_DBG	//printf
#define PRINT_ERR	printf


#define aml_writel32(val, reg)		(REG32(reg) = val)
#define aml_readl32(reg)		(REG32(reg))

xHandlerTableEntry xMbHandlerTable[IRQ_MAX];

void vMbDefaultHandler(void *vArg)
{
	vArg = vArg;
	return;
}

void vSetMbIrqHandler(unsigned int xNum, vHandlerFunc vHandler, void *vArg,
		    unsigned int xPriority)
{
	PRINT_DBG("vSetMbIrqHandler xNum:%d\n", xNum);
	if (vHandler == NULL) {
		xMbHandlerTable[xNum].vHandler = &vMbDefaultHandler;
		xMbHandlerTable[xNum].vArg = NULL;
	} else {
		xMbHandlerTable[xNum].vHandler = vHandler;
		xMbHandlerTable[xNum].vArg = vArg;
	}
	xMbHandlerTable[xNum].xPriority = xPriority;
}

void vMbSetIrqPriority(unsigned int xNum, unsigned int xPriority)
{
	xMbHandlerTable[xNum].xPriority = xPriority;
}

void vEnableMbInterrupt(unsigned int xMask)
{
	unsigned int val = 0;

	val = aml_readl32(MAILBOX_IRQ_MASK) | xMask;
	aml_writel32(val, MAILBOX_IRQ_MASK);
}

void vDisableMbInterrupt(unsigned int xMask)
{
	unsigned int val = 0;

	val = aml_readl32(MAILBOX_IRQ_MASK) & (~xMask);
	aml_writel32(val, MAILBOX_IRQ_MASK);
}

void vClrMbInterrupt(unsigned int xMask)
{
	unsigned int val = 0;

	val = aml_readl32(MAILBOX_IRQ_CLR) & (~xMask);
	aml_writel32(val, MAILBOX_IRQ_CLR);
}

unsigned int xGetMbIrqStats(void)
{
	unsigned int val = 0;

	val = aml_readl32(MAILBOX_IRQ_STS);
	return val;
}

