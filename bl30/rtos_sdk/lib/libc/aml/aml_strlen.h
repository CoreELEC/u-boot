/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_STRLEN_H__
#define __AML_STRLEN_H__

#ifdef __cplusplus
extern "C" {
#endif

#if (1 == CONFIG_ARM64)
#include <sys/types.h>

size_t  strlen (const char *s);
#else
int strlen(const char *s);
#endif

#ifdef __cplusplus
}
#endif

#endif

