/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_DMALLOC_EXT_H__
#define __AML_DMALLOC_EXT_H__

#include <stdint.h>

void vdRecordMalloc(size_t xWantedSize);
void vdRecordFree(size_t xFreeSize);
int vPrintDmallocInfo(size_t tid);
void xClearSpecDmallocNode(size_t tid);

#endif
