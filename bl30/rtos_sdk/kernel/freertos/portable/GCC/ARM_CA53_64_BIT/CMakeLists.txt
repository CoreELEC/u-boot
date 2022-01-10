# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_library_sources(
    port.c
	xlat_tables/xlat_tables_common.c
	xlat_tables/xlat_tables.c
    portASM.S
)

aml_library_include_directories(
	${CMAKE_CURRENT_LIST_DIR}
)
