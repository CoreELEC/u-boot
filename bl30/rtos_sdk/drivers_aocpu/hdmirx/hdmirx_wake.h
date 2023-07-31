/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __HDMIRX_WAKE__
#define __HDMIRX_WAKE__

#define INFO(fmt, args...) printf("[%s] " fmt "\n", __func__, ##args)
#define TOP_EDID_RAM_OVR0_DATA    ((volatile uint32_t *)(0xfe398000 + (0x016 << 2)))
void hdmirx_GpioIRQRegister(void);
void hdmirx_GpioIRQFree(void);


#endif
