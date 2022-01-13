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

#include <stdarg.h>		/* For va_list */
#include <stdint.h>
#include <stddef.h>

#include "common.h"
#include "FreeRTOSConfig.h"



int sPrintf_ext(char *str, size_t size, const char *format, va_list args);

int printk(const char *fmt, ...);




#ifdef __cplusplus
}
#endif

#endif				/* __CROS_EC_PRINTF_H */
