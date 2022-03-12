# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_add_library()

aml_library_include_directories(
	${SDK_BASE}/boards/${ARCH}/${BOARD}
)

add_subdirectory(${BOARD})

aml_library_link_libraries(kernel__${KERNEL})
aml_library_link_libraries(drivers)

aml_link_libraries()
