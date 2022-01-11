/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"

#include "mailbox-htbl.h"

#define PRINT(...)	//printf(__VA_ARGS__)
#define PRINT_ERR(...)	printf(__VA_ARGS__)

struct entry {
	uint32_t cmd;
	void *(*handler)(void *);
	uint8_t needFdBak;
	uint32_t tabLen;
};

static inline void *mbmemset(void *dst, int val, size_t count)
{
	char *ptr = dst;

	while (count--)
		*ptr++ = val;

	return dst;
}

static inline void *mbmemcpy(void *dst, const void *src, size_t len)
{
	const char *s = src;
	char *d = dst;

	while (len--)
		*d++ = *s++;

	return dst;
}

void mailbox_htbl_init(void **pHTbl)
{
	size_t size = sizeof(struct entry) * MAX_ENTRY_NUM;
	struct entry *p = NULL;

	p = malloc(size);
	if (p == NULL)
		return;
	mbmemset(p, 0x00, size);
	*pHTbl = p;
	p[0].tabLen = MAX_ENTRY_NUM;
}

void mailbox_htbl_init_size(void **pHTbl, uint32_t tabLen)
{
	size_t size = sizeof(struct entry) * tabLen;
	struct entry *p = NULL;

	if (tabLen == 0) {
		PRINT_ERR("tabLen == 0\n");
		return;
	}
	p = malloc(size);
	if (p == NULL)
		return;
	mbmemset(p, 0x00, size);
	*pHTbl = p;
	p[0].tabLen = tabLen;
}

uint32_t mailbox_htbl_reg(void *pHTbl, uint32_t cmd, void *(handler) (void *))
{
	struct entry *p = pHTbl;
	uint32_t i;
	uint32_t tabLen = p[0].tabLen;

	for (i = 0; i != tabLen; i++) {
		if (p[i].cmd == cmd && p[i].handler != NULL) {
			PRINT_ERR("FATAL ERROR: reg repeat cmd=%lx handler=%p\n", cmd, handler);
			for (;;);
		}
		if (p[i].handler == NULL) {
			p[i].cmd = cmd;
			p[i].handler = handler;
			p[i].needFdBak = 0;
			PRINT_ERR("AOCPU reg cmd=%lx handler=%p\n", cmd, handler);
			return i;
		}
	}
	return tabLen;
}

uint32_t mailbox_htbl_reg_feedback(void *pHTbl, uint32_t cmd,
				   void *(*handler) (void *), uint8_t needFdBak)
{
	struct entry *p = pHTbl;
	uint32_t i;
	uint32_t tabLen = p[0].tabLen;

	for (i = 0; i != tabLen; i++) {
		if (p[i].cmd == cmd && p[i].handler != NULL) {
			PRINT_ERR("FATAL ERROR: reg repeat cmd=%lx handler=%p\n", cmd,
				  handler);
			for (;;);
		}
		if (p[i].handler == NULL) {
			p[i].cmd = cmd;
			p[i].handler = handler;
			p[i].needFdBak = needFdBak;
			PRINT_ERR("reg idx=%ld cmd=%lx handler=%p\n", i, cmd, handler);
			return i;
		}
	}
	return tabLen;
}

uint32_t mailbox_htbl_unreg(void *pHTbl, uint32_t cmd)
{
	struct entry *p = pHTbl;
	uint32_t i;
	uint32_t tabLen = p[0].tabLen;

	for (i = 0; i != tabLen; i++) {
		if (p[i].cmd == cmd) {
			PRINT("unreg cmd=%lx handler=%p\n", cmd, p[i].handler);
			p[i].cmd = 0;
			p[i].handler = NULL;
			p[i].needFdBak = 0;
			return i;
		}
	}
	return MAX_ENTRY_NUM;
}

uint32_t mailbox_htbl_invokeCmd(void *pHTbl, uint32_t cmd, void *arg)
{
	PRINT("AOCPU search in cmd handler table pHTbl=%p cmd=%lx arg=%p\n", pHTbl,
	      cmd, arg);
	struct entry *p = pHTbl;
	uint32_t i;
	uint32_t tabLen = p[0].tabLen;

	for (i = 0; i != tabLen; i++) {
		PRINT("AOCPU input_cmd=%x i=%ld cmd=%lx\n", cmd, i, p[i].cmd);
		if (p[i].cmd == cmd) {
			PRINT("AOCPU idx=%ld cmd=%lx handler=%p arg=%p\n", i,
			      p[i].cmd, p[i].handler, arg);
			if (p[i].handler == NULL) {
				return tabLen;
			}
			p[i].handler(arg);
			return p[i].needFdBak;
		}
	}
	PRINT_ERR("AOCPU unknown cmd=%lx\n", cmd);
	return tabLen;
}
