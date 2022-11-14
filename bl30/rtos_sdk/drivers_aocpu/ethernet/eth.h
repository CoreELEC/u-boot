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
extern void vETHInit(uint32_t ulIrq, function_ptr_t handler);
extern void vETHDeint(void);
extern void eth_handler(void);
extern void vETHDeint_t5(void);
extern void eth_handler_t5(void);

#ifdef __cplusplus
}
#endif
#endif
