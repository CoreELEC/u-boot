# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_LIBC_STD)

aml_add_library()

aml_library_include_directories(
	"${CMAKE_CURRENT_LIST_DIR}"
)

aml_library_sources(
	syscalls.c
)

if(CONFIG_VFS)
aml_library_link_libraries(lib__vfs)
endif()
if(CONFIG_LOG_BUFFER)
aml_library_link_libraries(utilities__logbuffer)
endif()
aml_library_link_libraries(kernel__${KERNEL})
aml_library_link_libraries(drivers)

aml_link_libraries()

endif()

