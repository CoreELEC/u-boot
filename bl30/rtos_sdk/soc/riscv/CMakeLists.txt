# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_add_library()

aml_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

if (CONFIG_SOC_T5W OR CONFIG_SOC_T5D)
aml_library_sources(
	n200_func.c
	interrupt_control_pic.c
)
else()
aml_library_sources(
	n200_func.c
	interrupt_control_eclic.c
)
endif()

add_subdirectory(${SOC})

aml_library_link_libraries(arch__${ARCH})

aml_link_libraries()
