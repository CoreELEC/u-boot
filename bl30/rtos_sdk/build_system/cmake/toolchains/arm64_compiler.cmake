# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

include("${CMAKE_CURRENT_LIST_DIR}/find_compiler.cmake")

set(CMAKE_SYSTEM_NAME Generic)

if("${COMPILER}" STREQUAL "clang+llvm")
	# Find CLANG for ARM.
	aml_find_compiler(COMPILER_CC_CLANG clang)
	aml_find_compiler(COMPILER_CXX_CLANG clang++)
	aml_find_compiler(COMPILER_LD lld)
	message(STATUS "@@COMPILER_LD: ${COMPILER_LD}")
	message(STATUS "@@COMPILER_CC_CLANG: ${COMPILER_CC_CLANG}")
	message(STATUS "@@COMPILER_CXX_CLANG: ${COMPILER_CXX_CLANG}")
	set(COMPILER_ASM "${COMPILER_CC_CLANG}" CACHE INTERNAL "" FORCE)
	aml_find_compiler(COMPILER_OBJCOPY_CLANG llvm-objcopy)
	aml_find_compiler(COMPILER_OBJDUMP_CLANG llvm-objdump)

	# Specify the compiler.
	set(CMAKE_C_COMPILER ${COMPILER_CC_CLANG} CACHE FILEPATH "C compiler" FORCE)
	set(CMAKE_CXX_COMPILER ${COMPILER_CXX_CLANG} CACHE FILEPATH "C++ compiler" FORCE)
	set(CMAKE_ASM_COMPILER ${COMPILER_ASM} CACHE FILEPATH "ASM compiler" FORCE)
	set(CMAKE_LINKER ${COMPILER_LD} CACHE FILEPATH "LD linker" FORCE)
	set(CMAKE_OBJCOPY_COMPILER ${COMPILER_OBJCOPY_CLANG} CACHE FILEPATH "objcopy compiler" FORCE)
	set(CMAKE_OBJDUMP_COMPILER ${COMPILER_OBJDUMP_CLANG} CACHE FILEPATH "objdump compiler" FORCE)
	SET (CMAKE_OBJDUMP ${COMPILER_OBJDUMP_CLANG} CACHE FILEPATH "objdump compiler" FORCE)
	SET (CMAKE_OBJCOPY ${COMPILER_OBJCOPY_CLANG} CACHE FILEPATH "objcopy compiler" FORCE)
	# Disable compiler checks.
	set(CMAKE_C_COMPILER_FORCED TRUE FORCE)
	set(CMAKE_CXX_COMPILER_FORCED TRUE FORCE)

	# Add target system root to cmake find path.
	get_filename_component(COMPILER_DIR "${COMPILER_CC_CLANG}" DIRECTORY)
	get_filename_component(CMAKE_FIND_ROOT_PATH "${COMPILER_DIR}" DIRECTORY)

	# Look for includes and libraries only in the target system prefix.
	#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

	# Helper variables to abstracts some common compiler flags.
	set(COMPILER_NO_WARNINGS "-w" CACHE INTERNAL "" FORCE)
else()
	# Find GCC for ARM.
	aml_find_compiler(COMPILER_CC aarch64-none-elf-gcc)
	aml_find_compiler(COMPILER_CXX aarch64-none-elf-g++)
	set(COMPILER_ASM "${COMPILER_CC}" CACHE FILEPATH "" FORCE)
	aml_find_compiler(COMPILER_NM aarch64-none-elf-nm)
	aml_find_compiler(COMPILER_OBJCOPY aarch64-none-elf-objcopy)
	aml_find_compiler(COMPILER_OBJDUMP aarch64-none-elf-objdump)

	# Specify the compiler.
	set(CMAKE_C_COMPILER ${COMPILER_CC} CACHE FILEPATH "C compiler" FORCE)
	set(CMAKE_CXX_COMPILER ${COMPILER_CXX} CACHE FILEPATH "C++ compiler" FORCE)
	set(CMAKE_ASM_COMPILER ${COMPILER_ASM} CACHE FILEPATH "ASM compiler" FORCE)
	#set(CMAKE_C_LINK_EXECUTABLE ${COMPILER_LD} CACHE FILEPATH "LD linker" FORCE)
	set(CMAKE_NM_COMPILER ${COMPILER_NM} CACHE FILEPATH "nm compiler" FORCE)
	set(CMAKE_OBJCOPY_COMPILER ${COMPILER_OBJCOPY} CACHE FILEPATH "objcopy compiler" FORCE)
	set(CMAKE_OBJDUMP_COMPILER ${COMPILER_OBJDUMP} CACHE FILEPATH "objdump compiler" FORCE)
	# Disable compiler checks.
	set(CMAKE_C_COMPILER_FORCED TRUE FORCE)
	set(CMAKE_CXX_COMPILER_FORCED TRUE FORCE)

	# Add target system root to cmake find path.
	get_filename_component(COMPILER_DIR "${COMPILER_CC}" DIRECTORY)
	get_filename_component(CMAKE_FIND_ROOT_PATH "${COMPILER_DIR}" DIRECTORY)

	# Look for includes and libraries only in the target system prefix.
	#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
	#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

	# Helper variables to abstracts some common compiler flags.
	set(COMPILER_NO_WARNINGS "-w" CACHE INTERNAL "" FORCE)
endif()
