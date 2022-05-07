/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _MAILBOX_HTBL_H_
#define _MAILBOX_HTBL_H_

#include <stdint.h>
#include "mailbox-api.h"

/** handler table
 * 0, mailbox init one htbl
 * 1, user could register it's handler on specific cmd to htbl
 * 2, mailbox invoke handler when specific cmd come from peer core
 *
 * For now, ARM and DSP have only one htbl at each side.
 */
typedef void *(*handler_t)(void *);

void mailbox_htbl_init(void **pHTbl);
void mailbox_htbl_init_size(void **pHTbl, uint32_t tabLen);
uint32_t mailbox_htbl_reg(void *pHTbl, uint32_t cmd, handler_t handler);
uint32_t mailbox_htbl_reg_feedback(void *pHTbl, uint32_t cmd, handler_t handle,
				   uint8_t needFdBak);
uint32_t mailbox_htbl_unreg(void *pHTbl, uint32_t cmd);
uint32_t mailbox_htbl_invokeCmd(void *pHTbl, uint32_t cmd, void *arg);

/** dispatch mailbox msg from ISR to job task with this struct
 * sample user case:
 * In ISR:
 * 1, get msg(cmd and data) from mailbox, create event
 * 2, push event to Q in ISR
 * 3, notify the job task in ISR
 * In job task:
 * 1, wait until be notified from ISR
 * 2, recieve event from Q
 * 3, registered handler process the event
 * 4, waiting for next event
 */
// ARM side don't support malloc in ISR,
// so change to fixed payload style
// defect:
// - fixed payload length
// - it have to copy to Q when pushing, copy from Q when poping
#define PAYLOAD_LEN 24
struct event {
	uint32_t cmd;
	uint8_t arg[PAYLOAD_LEN];
};
#endif
