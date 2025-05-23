# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_STACK_PROTECTOR_STRONG)

set(CURRENT_LIBRARY_TYPE STATIC)
set(CURRENT_LIBRARY libssp_nonshared)
aml_add_library_spec(${CURRENT_LIBRARY} ${CURRENT_LIBRARY_TYPE})

aml_library_include_directories(
	"${CMAKE_CURRENT_LIST_DIR}"
)

aml_library_sources(
	ssp_nonshared.c
)

aml_link_libraries()

endif()

