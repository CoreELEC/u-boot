# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_SOC_TXHD2)
aml_library_sources_ifdef(
    CONFIG_DDR
    ddr_txhd2.c
)
else()
aml_library_sources_ifdef(
    CONFIG_DDR
    ddr.c
)
endif()

