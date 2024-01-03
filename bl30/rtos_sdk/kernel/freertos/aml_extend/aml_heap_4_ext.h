/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_HEAP_4_EXT_H__
#define __AML_HEAP_4_EXT_H__

#include <stddef.h>

size_t xPortGetTotalHeapSize( void );

#ifdef configRealloc
void *xPortRealloc(void *ptr, size_t size);
#endif

#endif
