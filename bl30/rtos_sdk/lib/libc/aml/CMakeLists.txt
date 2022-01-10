# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_LIBC_AML)

aml_add_library()

aml_library_sources(
	aml_scanf.c
	aml_printf.c
	aml_malloc.c
	aml_free.c
	aml_memcpy.c
	aml_memset.c
	aml_strlen.c
	)

aml_library_link_libraries(kernel__${KERNEL})
aml_library_link_libraries(drivers)
aml_link_libraries()

endif()

