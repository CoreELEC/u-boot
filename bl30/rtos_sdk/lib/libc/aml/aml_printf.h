/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_PRINTF_H__
#define __AML_PRINTF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h> /* For va_list */
#include <stdint.h>
#include <stddef.h>

#include "common.h"
#include "FreeRTOSConfig.h"

#define KERN_ERROR 1
#define KERN_WARNING 2
#define KERN_DEBUG 3
#define KERN_INFO 4

#ifndef TAG
#define TAG
#endif

#if CONFIG_LOGLEVEL_DEFAULT >= 1
#define AGLOGE(format, ...) LOGE(KERN_ERROR, TAG format, ##__VA_ARGS__)
#else
#define AGLOGE(format, ...)
#endif

#if CONFIG_LOGLEVEL_DEFAULT >= 2
#define AGLOGW(format, ...) LOGW(KERN_WARNING, TAG format, ##__VA_ARGS__)
#else
#define AGLOGW(format, ...)
#endif

#if CONFIG_LOGLEVEL_DEFAULT >= 3
#define AGLOGD(format, ...) LOGD(KERN_DEBUG, TAG format, ##__VA_ARGS__)
#else
#define AGLOGD(format, ...)
#endif

int vfnprintf(int (*addchar)(void *context, int c), void *context, const char *format,
	      va_list args);

int sPrintf_ext(char *str, size_t size, const char *format, va_list args);
int printf(const char *fmt, ...);
int iprintf(const char *fmt, ...);
int printk(const char *fmt, ...);
int sPrintf(char *str, size_t size, const char *fmt, ...);
int syslog_buf_init(void);
int iprint_string(char *str);

#ifdef __cplusplus
}
#endif

#endif
