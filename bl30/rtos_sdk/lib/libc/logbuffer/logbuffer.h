/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _LOGBUF_H_
#define _LOGBUF_H_

#ifdef __cplusplus
extern "C" {
#endif

int logbuf_setting(uint32_t pa, uint32_t len);
void logbuf_enable(void);
void logbuf_disable(void);
int logbuf_is_enable(void);
int logbuf_output_str(const char *str);
int logbuf_output_len(const char *buf, int len);
int logbuf_output_char(const char ch);
void logbuffer_init(void);

#ifdef __cplusplus
}
#endif
#endif
