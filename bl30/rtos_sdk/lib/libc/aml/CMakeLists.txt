# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_LIBC_AML)

aml_add_library()

aml_library_sources(
	aml_scanf.c
	aml_putchar.c
	aml_printf.c
	aml_malloc.c
	aml_free.c
	aml_atoi.c
	aml_errno.c
	aml_isalpha.c
	aml_isdigit.c
	aml_islower.c
	aml_isspace.c
	aml_isupper.c
	aml_tolower.c
	aml_memcpy.c
	aml_memset.c
	aml_memmove.c
	aml_memcmp.c
	aml_strlen.c
	aml_strncpy.c
	aml_strtoul.c
	aml_strcasecmp.c
	aml_strncasecmp.c
	)

aml_library_link_libraries(kernel__${KERNEL})
aml_library_link_libraries(drivers)
aml_link_libraries()

endif()

