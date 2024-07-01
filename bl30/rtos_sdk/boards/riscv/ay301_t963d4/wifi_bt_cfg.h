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

//#define WIFI_WAKE_CFG 0
#define WIFI_WAKE_HOST GPIOD_12
#define WIFI_PWREN GPIOD_11

#define BT_WAKE_CFG 1
#define BT_WAKE_HOST GPIOD_8
#define BT_EN GPIOD_11

#endif

#ifdef __cplusplus
}
#endif

#endif
