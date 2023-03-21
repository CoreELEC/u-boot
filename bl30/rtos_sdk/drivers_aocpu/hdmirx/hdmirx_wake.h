/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __HDMIRX_WAKE__
#define __HDMIRX_WAKE__

#define INFO(fmt, args...) printf("[%s] " fmt "\n", __func__, ##args)

void hdmirx_GpioIRQRegister(void);
void hdmirx_GpioIRQFree(void);


#endif
