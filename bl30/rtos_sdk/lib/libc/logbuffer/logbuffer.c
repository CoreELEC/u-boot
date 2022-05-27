/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#ifdef CONFIG_ARM64
#include "initcall.h"
#endif

#define LOGBUF_RDFLG 0x80000000

#define LOGBUF_STAT_NOT_INITED 0
#define LOGBUF_STAT_DISABLED 1
#define LOGBUF_STAT_ENABLED 2

struct logbuf {
	uint32_t write;
	uint32_t read;
	char buf[0];
};

static char *logbuf;
static int logbuf_stat;
static uint32_t logbuf_len;
volatile struct logbuf *plogbuf;

int logbuf_setting(uint32_t pa, uint32_t len)
{
	uint32_t buf = ((pa + 3) & ~0x3);

	plogbuf = (struct logbuf *)(unsigned long)buf;
	len = len - (buf - pa);
	if (len <= sizeof(struct logbuf))
		return -1;
	logbuf_len = len - sizeof(struct logbuf);
	plogbuf->read = 0;
	plogbuf->write = 0;
	vPortConfigLogBuf(buf, len);
	logbuf_stat = LOGBUF_STAT_DISABLED;
	return 0;
}

void logbuf_enable(void)
{
	if (logbuf_stat == LOGBUF_STAT_DISABLED)
		logbuf_stat = LOGBUF_STAT_ENABLED;
}

void logbuf_disable(void)
{
	if (logbuf_stat == LOGBUF_STAT_ENABLED)
		logbuf_stat = LOGBUF_STAT_DISABLED;
}

int logbuf_is_enable(void) { return (logbuf_stat == LOGBUF_STAT_ENABLED); }

static int _logbuf_output_len(const char *buf, int len)
{
	unsigned long flags;
	uint32_t rd, rf, wt;
	int ret = 0, cnt1, cnt2, remain = len;

	portIRQ_SAVE(flags);
	while (remain > 0) {
		wt = plogbuf->write;
		cnt1 = logbuf_len - wt;
		if (cnt1 >= (int)logbuf_len)
			cnt1--;
		rd = plogbuf->read;
		portMEMORY_BARRIER();
		rf = (rd & LOGBUF_RDFLG);
		rd = (rd & ~LOGBUF_RDFLG);
		if (remain < cnt1)
			cnt1 = remain;
		if (wt < rd)
			cnt2 = rd - wt - 1;
		else
			cnt2 = rd + logbuf_len - wt - 1;
		if (cnt2 > 0) {
			if (cnt1 > cnt2)
				cnt1 = cnt2;
			memcpy((char *)plogbuf->buf + wt, buf + ret, cnt1);
			ret += cnt1;
			remain -= cnt1;
			wt += cnt1;
			if (wt >= logbuf_len)
				wt -= logbuf_len;
			plogbuf->write = wt;
		} else {
			if (rf)
				continue;
			cnt2 = rd + cnt1;
			if (cnt2 >= (int)logbuf_len)
				cnt2 -= logbuf_len;
			portCMPXCHG(&(plogbuf->read), rd, cnt2);
		}
	}
	portIRQ_RESTORE(flags);
	return ret;
}

int logbuf_output_str(const char *str)
{
	int len;

	if (logbuf_stat != LOGBUF_STAT_ENABLED)
		return 0;
	len = strlen(str);
	return _logbuf_output_len(str, len);
}

int logbuf_output_len(const char *buf, int len)
{
	if (logbuf_stat != LOGBUF_STAT_ENABLED)
		return 0;
	return _logbuf_output_len(buf, len);
}

int logbuf_output_char(const char ch)
{
	if (logbuf_stat != LOGBUF_STAT_ENABLED)
		return 0;
	return _logbuf_output_len(&ch, 1);
}

void logbuffer_init(void)
{
	if (!logbuf_is_enable()) {
		logbuf = malloc(CONFIG_LOG_BUFFER_CACHE_LEN);
		if (logbuf) {
			logbuf_setting((uint32_t)(uint64_t)logbuf,
				       CONFIG_LOG_BUFFER_CACHE_LEN);
			logbuf_enable();
		}
	}
}

#ifdef CONFIG_ARM64
SERVICE_INIT(logbuffer_init);
#endif
