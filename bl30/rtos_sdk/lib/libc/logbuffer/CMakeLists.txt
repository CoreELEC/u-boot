# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_LOG_BUFFER)

aml_add_library()

aml_library_include_directories(
	"${CMAKE_CURRENT_LIST_DIR}"
)
aml_library_sources(
    logbuffer.c
)

aml_library_link_libraries(kernel__${KERNEL})

aml_link_libraries()

endif()