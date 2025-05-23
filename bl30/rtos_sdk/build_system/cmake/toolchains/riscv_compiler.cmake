# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

set(SPLIT_ARCH_DIR $ENV{SPLIT_ARCH_DIR})
if(NOT SPLIT_ARCH_DIR)
    set(SOC_DIR $ENV{SDK_BASE}/soc/$ENV{ARCH}/$ENV{SOC})
else()
    set(SOC_DIR $ENV{SDK_BASE}/soc/$ENV{ARCH}/${SPLIT_ARCH_DIR}/$ENV{SOC})
endif()

if(EXISTS ${SOC_DIR}/toolchain_path.cmake)
    include(${SOC_DIR}/toolchain_path.cmake)
    # This information is provided by U-Boot and includes the toolchain internally.
    get_filename_component(PREBUILD_DIR $ENV{SDK_BASE} DIRECTORY)
    get_filename_component(PREBUILD_DIR ${PREBUILD_DIR} DIRECTORY)
    if(EXISTS ${PREBUILD_DIR}/prebuild/riscv-none-gcc AND IS_DIRECTORY ${PREBUILD_DIR}/prebuild/riscv-none-gcc)
        get_filename_component(LAST_COMPONENT $ENV{TOOLCHAIN_PATH} NAME)
        set(ENV{TOOLCHAIN_PATH} ${PREBUILD_DIR}/prebuild/riscv-none-gcc/${LAST_COMPONENT})
    endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/find_compiler.cmake")

set(CMAKE_SYSTEM_NAME Generic)

# Find GCC for RISCV.
aml_find_compiler(COMPILER_CC riscv-none-embed-gcc)
aml_find_compiler(COMPILER_CXX riscv-none-embed-g++)
set(COMPILER_ASM "${COMPILER_CC}" CACHE INTERNAL "")
aml_find_compiler(COMPILER_NM riscv-none-embed-nm)
aml_find_compiler(COMPILER_OBJCOPY riscv-none-embed-objcopy)
aml_find_compiler(COMPILER_OBJDUMP riscv-none-embed-objdump)

# Specify the cross compiler.
set(CMAKE_C_COMPILER ${COMPILER_CC} CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER ${COMPILER_CXX} CACHE FILEPATH "C++ compiler")
set(CMAKE_ASM_COMPILER ${COMPILER_ASM} CACHE FILEPATH "ASM compiler")
#set(CMAKE_C_LINK_EXECUTABLE ${COMPILER_LD} CACHE FILEPATH "LD linker")
set(CMAKE_NM_COMPILER ${COMPILER_NM} CACHE FILEPATH "nm compiler")
set(CMAKE_OBJCOPY_COMPILER ${COMPILER_OBJCOPY} CACHE FILEPATH "objcopy compiler")
set(CMAKE_OBJDUMP_COMPILER ${COMPILER_OBJDUMP} CACHE FILEPATH "objdump compiler")
# Disable compiler checks.
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Add target system root to cmake find path.
get_filename_component(COMPILER_DIR "${COMPILER_CC}" DIRECTORY)
get_filename_component(CMAKE_FIND_ROOT_PATH "${COMPILER_DIR}" DIRECTORY)

# Look for includes and libraries only in the target system prefix.
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

# Helper variables to abstracts some common compiler flags.
set(COMPILER_NO_WARNINGS "-w" CACHE INTERNAL "")
