# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_add_library()

if(CONFIG_EXCEPTION_SVC)
target_compile_options(${CURRENT_LIBRARY} PUBLIC -DGUEST)
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

aml_library_include_directories(
	include
	portable/${CONFIG_KERNEL_COMPILER_DIR}/${CONFIG_KERNEL_ARCH_DIR}
	aml_extend
)

aml_library_link_libraries(arch__${ARCH})
aml_library_link_libraries(drivers)

if(CONFIG_BACKTRACE)
aml_library_link_libraries(lib__backtrace)
endif()
if(CONFIG_BARECTF)
aml_library_link_libraries(utilities__barectf)
endif()

aml_link_libraries()
