/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_MEMCPY_H__
#define __AML_MEMCPY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "common.h"

void *memcpy(void *dest, const void *src, size_t len);

#ifdef __cplusplus
}
#endif

#endif
