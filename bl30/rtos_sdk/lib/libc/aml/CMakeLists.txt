# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_LIBC_AML)

aml_add_library()

aml_library_sources(
	aml_cvt.c
	aml_scanf.c
	aml_putchar.c
	aml_fprintf.c
	aml_printf.c
	aml_malloc.c
	aml_free.c
	aml_atoi.c
	aml_errno.c
	aml_impure.c
	aml_isalpha.c
	aml_isdigit.c
	aml_islower.c
	aml_isspace.c
	aml_isupper.c
	aml_tolower.c
	aml_strcmp.c
	aml_strlen.c
	aml_strnlen.c
	aml_strncpy.c
	aml_strtoul.c
	aml_strcasecmp.c
	aml_strncasecmp.c
	)

if(CONFIG_ARM64)
#memmove is a alias function as memcpy in aml_memcpy.S
aml_library_sources(
	asm/${ARCH}/aml_memcpy.S
	asm/${ARCH}/aml_memset.S
	asm/${ARCH}/aml_memcmp.S
	)
else()
aml_library_sources(
	aml_memcpy.c
	aml_memset.c
	aml_memmove.c
	aml_memcmp.c
	)
endif()

aml_library_link_libraries(arch__${ARCH})
aml_library_link_libraries(drivers)
aml_library_link_libraries(kernel__${KERNEL})

aml_link_libraries()

endif()

