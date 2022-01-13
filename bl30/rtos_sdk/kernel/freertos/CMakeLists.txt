# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_add_library()

if(CONFIG_ARM64)
target_compile_options(${CURRENT_LIBRARY} PUBLIC -DARCH64)
endif()

aml_library_sources(
	list.c
	queue.c
	tasks.c
	event_groups.c
	timers.c
)

aml_library_sources(
	portable/${CONFIG_KERNEL_COMPILER_DIR}/${CONFIG_KERNEL_ARCH_DIR}/port.c
	portable/${CONFIG_KERNEL_COMPILER_DIR}/${CONFIG_KERNEL_ARCH_DIR}/portASM.S
)

if(CONFIG_XTENSA)
aml_library_sources(
	portable/MemMang/heap_4.c
	portable/${CONFIG_KERNEL_COMPILER_DIR}/${CONFIG_KERNEL_ARCH_DIR}/portclib.c
)
else()
aml_library_sources(
	portable/MemMang/heap_5.c
)
endif()

if(CONFIG_ARM_CA53_64_BIT)
aml_library_sources(
	portable/${CONFIG_KERNEL_COMPILER_DIR}/${CONFIG_KERNEL_ARCH_DIR}/xlat_tables/xlat_tables_common.c
	portable/${CONFIG_KERNEL_COMPILER_DIR}/${CONFIG_KERNEL_ARCH_DIR}/xlat_tables/xlat_tables.c
)

aml_library_include_directories(
	${SDK_BASE}/libs/stack_trace
)
endif()

aml_library_include_directories(
	include
	portable/${CONFIG_KERNEL_COMPILER_DIR}/${CONFIG_KERNEL_ARCH_DIR}
)

aml_library_link_libraries(arch__${ARCH})
aml_library_link_libraries(drivers)

aml_link_libraries()
