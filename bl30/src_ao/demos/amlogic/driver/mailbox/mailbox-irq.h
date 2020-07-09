#ifndef _MAILBOX_IRQ_H_
#define _MAILBOX_IRQ_H_
#include "mailbox.h"

void vSetMbIrqHandler(unsigned int xNum, vHandlerFunc vHandler, void *vArg, unsigned int xPriority);
void vMbSetIrqPriority(unsigned int xNum, unsigned int xPriority);
void vEnableMbInterrupt(unsigned int xMask);
void vDisableMbInterrupt(unsigned int xMask);
void vClrMbInterrupt(unsigned int xMask);
unsigned int xGetMbIrqStats(void);
#endif

