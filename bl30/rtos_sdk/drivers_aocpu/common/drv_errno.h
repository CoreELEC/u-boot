/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _DRV_ERRNO_H_
#define _DRV_ERRNO_H_

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ERRNO_DRV_START 0x80

/* driver General error codes */
enum{
	DRV_ERROR = ERRNO_DRV_START,    // Unspecified error
	DRV_ERROR_ACCESS,              // Invalid access
	DRV_ERROR_ALIGN,               // Invalid alignment
	DRV_ERROR_BUSY,                // Driver is busy
	DRV_ERROR_IQR,                 // IQRnumber error
	DRV_ERROR_PARAMETER,           // Parameter error
	DRV_ERROR_PWRSTATE,	           // Invalid power state
	DRV_ERROR_TIMEOUT,             // Timeout occurred
	DRV_ERROR_SIZE,                // Invalid size
	DRV_ERROR_UNSUPPORTED,         // Operation not supported
	DRV_ERROR_SPECIFIC             // Start of driver specific errors
};


/** Get error type */
#define GET_ERROR_TYPE(errno) \
	(error & 0xFF000000 >> 24)
/** Get error module */
#define GET_ERROR_MODULE(error) \
	(error & 0x00FF0000 >> 16)
/** Get error API */
#define GET_ERROR_API(error) \
	(error & 0x0000FF00 >> 8)
/** Get errno */
#define GET_ERROR_NUM(error) \
	(error & 0x000000FF)

#ifndef DRV_ERRNO_BASE
/** means bsp errors */
#define DRV_ERRNO_BASE             0x81000000
#endif

/** driver module id definition*/
#define DRV_ERRNO_MAILBOX_BASE     0x81010000
#define DRV_ERRNO_IR_BASE        0x81020000
#define DRV_ERRNO_I2C_BASE         0x81030000

int amlDrvGetErrno(void);
void amlDrvSetErrno(int no);


#ifdef __cplusplus
}
#endif

#endif /* _DRV_ERRNO_H_ */
