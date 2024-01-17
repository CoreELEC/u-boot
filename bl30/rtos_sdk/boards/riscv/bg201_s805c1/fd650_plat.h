/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include <gpio.h>
#include <fd650.h>

struct fd650_bus fd650_plat_data[] = {
	{0, 10, GPIOD_3, GPIOD_4},
};
