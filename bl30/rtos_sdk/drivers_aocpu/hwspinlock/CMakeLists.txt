# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_SOC_S5)
aml_library_sources_ifdef(
	CONFIG_HWSPINLOCK
	hwspinlock.c
)
endif()
