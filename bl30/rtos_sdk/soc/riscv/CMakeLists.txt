# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_add_library()

aml_library_include_directories(
	${SDK_BASE}/soc/${ARCH}
	${SDK_BASE}/soc/${ARCH}/${SOC}
)

aml_library_sources(
	n200_func.c
)

aml_library_link_libraries(arch__${ARCH})

aml_link_libraries()
