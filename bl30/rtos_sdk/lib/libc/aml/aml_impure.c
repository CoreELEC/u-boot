/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "aml_impure.h"

#ifndef __ATTRIBUTE_IMPURE_PTR__
#define __ATTRIBUTE_IMPURE_PTR__
#endif

#ifndef __ATTRIBUTE_IMPURE_DATA__
#define __ATTRIBUTE_IMPURE_DATA__
#endif

/* const struct used in _REENT_INIT */
#if defined(_REENT_SMALL) && !defined(_REENT_GLOBAL_STDIO_STREAMS)
const struct __sFILE_fake __sf_fake_stdin;
const struct __sFILE_fake __sf_fake_stdout;
const struct __sFILE_fake __sf_fake_stderr;
#endif

static struct _reent __ATTRIBUTE_IMPURE_DATA__ aml_impure_data = _REENT_INIT(aml_impure_data);
struct _reent *__ATTRIBUTE_IMPURE_PTR__ _impure_ptr = &aml_impure_data;

void _reclaim_reent(struct _reent *reent_ptr)
{
	/* do nothing now */
	(void)reent_ptr;
}
