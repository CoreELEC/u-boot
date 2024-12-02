# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_add_library()

aml_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(CONFIG_RISCV_WCN)
aml_library_sources(
	start_wcn.S
	entry_wcn.S
	vector_wcn_ram.S
	vector_wcn_rom.S
	exception_handler.c
)
else()
aml_library_sources(
	start.S
	vtable.S
	handlers.c
)

redefine_file_macro()

target_compile_options(
	${CURRENT_LIBRARY}
	PRIVATE
	-Wno-builtin-macro-redefined
)
endif()

aml_library_link_libraries(kernel__${KERNEL})
if(CONFIG_DEBUG_COREDUMP)
aml_library_link_libraries(utilities__coredump)
endif()

aml_link_libraries()
