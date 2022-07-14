/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_MED_EXT_H__
#define __AML_MED_EXT_H__

#include <stdint.h>

#ifdef CONFIG_MEMORY_ERROR_DETECTION
int xCheckMallocNodeIsOver(void *node);
int xPortCheckIntegrity(void);
int xPortMemoryScan(void);
#endif

#endif
