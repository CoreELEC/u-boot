/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_STRNLEN_H__
#define __AML_STRNLEN_H__

#ifdef __cplusplus
extern "C" {
#endif

#if (1 == CONFIG_ARM64)
#include <sys/types.h>

size_t strnlen(const char *s, size_t maxlen);
#else
int strnlen(const char *s, int maxlen);
#endif

#ifdef __cplusplus
}
#endif

#endif
