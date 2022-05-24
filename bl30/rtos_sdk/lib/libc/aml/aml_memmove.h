/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_MEMMOVE_H__
#define __AML_MEMMOVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void *memmove(void *dest, const void *src, size_t n);

#ifdef __cplusplus
}
#endif

#endif
