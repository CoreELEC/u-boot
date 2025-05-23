# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_library_sources(
	portASM.S
	port.c
)

aml_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/chip_specific_extensions/RV32I_CLINT_no_extensions
)
