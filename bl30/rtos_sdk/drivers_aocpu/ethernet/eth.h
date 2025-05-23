/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __ETH_H__
#define __ETH_H__

#ifdef __cplusplus
extern "C" {
#endif
extern void vETHInit(uint32_t type);
extern void vETHDeint(void);
extern void vETHEnableIrq(void);
extern int get_ETHWol_flag(void);
extern void vETHMailboxCallback(void);
#ifdef __cplusplus
}
#endif
#endif
