# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_LIBC_STD)

aml_add_library()

aml_library_sources(
	syscalls.c
	printk.c
)

aml_library_link_libraries(kernel__${KERNEL})
aml_library_link_libraries(drivers)
target_link_libraries(${TARGET_NAME} -lc)
target_link_libraries(${TARGET_NAME} libc__newlib)
target_link_libraries(${TARGET_NAME} -lc)

endif()

