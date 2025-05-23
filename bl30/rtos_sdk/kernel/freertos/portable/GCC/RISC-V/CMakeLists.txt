# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_library_sources(
	start.S
	vtable.S
	portasm.S
	port.c
)

aml_library_include_directories(
	${CMAKE_CURRENT_LIST_DIR}
)
