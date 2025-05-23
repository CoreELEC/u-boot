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

int printf(const char *fmt, ...);

int iprintf(const char *fmt, ...);

int puts(const char *str);

#ifdef __cplusplus
}
#endif

#endif
