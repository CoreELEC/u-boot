# Copyright (c) 2023-2024 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_LIBC_TEST)

aml_add_library()

set(test_dir "${CMAKE_CURRENT_LIST_DIR}")

aml_library_sources(
    "${test_dir}/test_libc_str.c"
    "${test_dir}/test_libc_chr.c"
)

aml_library_link_libraries(lib__unity)

# aml_library_link_libraries(lib__libc)

aml_library_link_libraries(kernel__${KERNEL})

aml_link_libraries()

endif()
