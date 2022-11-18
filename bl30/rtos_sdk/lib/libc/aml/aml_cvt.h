/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AML_CVT_H__
#define __AML_CVT_H__

#ifdef __cplusplus
extern "C" {
#endif

char *ecvt(double arg, int ndigits, int *decpt, int *sign);

char *my_fcvt(double arg, int ndigits, int *decpt, int *sign);

#ifdef __cplusplus
}
#endif

#endif
