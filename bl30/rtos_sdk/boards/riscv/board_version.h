/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BOARD_VERSION_H__
#define __BOARD_VERSION_H__

void output_aocpu_info(void);

#ifdef CONFIG_BL30_VERSION_SAVE
int bl30_plat_save_version(void);
#endif

#endif /* __BOARD_VERSION_H__ */
