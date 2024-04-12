/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __WIFI_BT_CFG_H__
#define __WIFI_BT_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

//#include "gpio.h"

#if CONFIG_WIFI_BT_WAKE

#define WIFI_WAKE_CFG 1
#define WIFI_WAKE_HOST GPIOX_7
#define WIFI_PWREN GPIOX_6

#define BT_WAKE_CFG 1
#define BT_WAKE_HOST GPIOX_18
#define BT_EN GPIOX_17

#endif

#ifdef __cplusplus
}
#endif

#endif

