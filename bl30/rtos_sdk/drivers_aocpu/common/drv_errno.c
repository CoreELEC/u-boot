/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/* global errno for driver */
static volatile int __aml_drv_errno;

int amlDrvGetErrno(void)
{
	return __aml_drv_errno;
}

void amlDrvSetErrno(int error)
{
	__aml_drv_errno = error;
}
