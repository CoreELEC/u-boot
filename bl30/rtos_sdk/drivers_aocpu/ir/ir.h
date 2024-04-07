/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _IR_COMMON_H
#define _IR_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio.h"

struct IRPowerKey {
	uint32_t code;
	uint8_t type;
};

enum PowerKeyType { IR_NORMAL, IR_CUSTOM };

/*supported protocol*/
#define MODE_SOFT 0x0
#define MODE_HARD_NEC 0x1
#define MODE_HARD_NEC_32K	0x10
#define MODE_HARD_LEAGCY_NEC 0xff
#define MAX_KEY_NUM 80

/**
 * Other protocol used by customer, due to size of firmware will not enabled
 * by default
 */

//#define MODE_HARD_DUOKAN	0x02
//#define MODE_HARD_XMP_1	0x03
//#define MODE_HARD_RC5		0x04
//#define MODE_HARD_RC6		0x05
//#define MODE_HARD_TOSHIBA	0x06
//#define MODE_HARD_RCA		0x08
//#define MODE_HARD_RCMM	0x09

/* sample for multi-protocol */
/* #define MODE_HARD_RCA_NEC	(MODE_HARD_RCA | MODE_HARD_LEAGCY_NEC << 8) */

/* for 2 multi-format controller */
/* #define MODE_HARD_NEC_NEC	(MODE_HARD_NEC | MODE_HARD_NEC << 8) */

/**
 *  xIRInit() - IR hardware initialize.
 *  @usWorkMode: supported protocol.
 *  @usGpio: which gpio is used as input.
 *  @func: function number of gpio use as ir input.
 */
extern uint32_t vIRInit(uint16_t usWorkMode, uint16_t usGpio, enum PinMuxType func,
		    struct IRPowerKey *ulPowerKeyList, uint8_t ucPowerKeyNum,
		    void (*vIRHandler)(struct IRPowerKey *pkey));

/**
 *  vInitIRWorkMode() - change ir protocol.
 *  @usWorkMode - protocol(0:soft, 1: hard nec)
 */
extern void vInitIRWorkMode(uint16_t usWorkMode);

/**
 *   vIsIRDebugEnable() - open/close ir driver debug
 *   @ucEnable: 1 enable 0 disable debug message.
 */
extern void vIsIRDebugEnable(uint8_t ucEnable);

/**
 *  ucIsIRInit() - use to see if ir driver is init.
 *  return 1 :already init  0: not ready.
 */
extern int8_t ucIsIRInit(void);

/**
 *  vIRDeint() - deinit ir
 */
extern void vIRDeint(void);
extern uint32_t vIRMailboxEnable(void);
extern void vIRGetKeyCode(struct IRPowerKey *ulPowerKeyList);
extern void vIR32KInit(uint32_t ulFrame0, uint32_t ulFrame1);
#ifdef __cplusplus
}
#endif
#endif
