# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if((CONFIG_SOC_S5) OR (CONFIG_SOC_T5M) OR (CONFIG_SOC_T3X))
    aml_library_sources_ifdef(
        CONFIG_HWSPINLOCK
        hwspinlock.c
    )
elseif((CONFIG_SOC_A4) OR (CONFIG_SOC_A5))
     aml_library_sources_ifdef(
        CONFIG_HWSPINLOCK
        hwspinlock_bakery.c
    )
endif()
