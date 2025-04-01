# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13.1)

set(SDK_BASE $ENV{SDK_BASE})
set(ARCH $ENV{ARCH})
set(SOC $ENV{SOC})
set(BOARD $ENV{BOARD})
set(CROSS_COMPILER $ENV{CROSS_COMPILER})
set(KERNEL $ENV{KERNEL})
set(PRODUCT $ENV{PRODUCT})
set(COMPILER $ENV{COMPILER})

set(SPLIT_ARCH_DIR $ENV{SPLIT_ARCH_DIR})
if(NOT SPLIT_ARCH_DIR)
    set(SOC_DIR ${SDK_BASE}/soc/${ARCH}/${SOC})
    set(BOARD_DIR ${SDK_BASE}/boards/${ARCH}/${BOARD})
else()
    set(SOC_DIR ${SDK_BASE}/soc/${ARCH}/${SPLIT_ARCH_DIR}/${SOC})
    set(BOARD_DIR ${SDK_BASE}/boards/${ARCH}/${SPLIT_ARCH_DIR}/${BOARD})
endif()
set(SDK_OUT ${CMAKE_BINARY_DIR})
set(PROJECT_BINARY_DIR ${CMAKE_BINARY_DIR})
set(PROJECT_SOURCE_DIR ${SDK_BASE})
set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ARCH_DIR ${SDK_BASE}/arch/${ARCH})
set(CMAKE_TOOLCAHIN_DIR ${SDK_BASE}/build_system/cmake/toolchains)

set(COLLECT_LINK_LIBRARIES ""  CACHE INTERNAL "")
set(COLLECT_LINK_OBJS ""  CACHE INTERNAL "")
set(COLLECT_IS_CPP_LIBRARIES "0"  CACHE INTERNAL "")

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

if(EXISTS   ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)
    set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)
elseif(EXISTS   ${APPLICATION_SOURCE_DIR}/prj.conf)
    set(CONF_FILE ${APPLICATION_SOURCE_DIR}/prj.conf)
endif()

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

include(${SDK_BASE}/build_system/cmake/extensions.cmake)
include(${SDK_BASE}/build_system/cmake/python.cmake)
include(${SDK_BASE}/build_system/cmake/kconfig.cmake)

set(TARGET_NAME $ENV{KERNEL})

# Do not prefix the output library file.
set(CMAKE_STATIC_LIBRARY_PREFIX "")

set(CMAKE_C_FLAGS "-imacros${AUTOCONF_H}")
set(CMAKE_CXX_FLAGS "-imacros${AUTOCONF_H}")
set(CMAKE_ASM_FLAGS "-imacros${AUTOCONF_H}")

if(EXISTS ${ARCH_DIR}/compiler_options.cmake)
    include(${ARCH_DIR}/compiler_options.cmake)
endif()
if(EXISTS ${SOC_DIR}/compiler_options.cmake)
    include(${SOC_DIR}/compiler_options.cmake)
endif()
if(EXISTS ${BOARD_DIR}/compiler_options.cmake)
    include(${BOARD_DIR}/compiler_options.cmake)
endif()

# The compile options which are differ with toolchain should in ${ARCH}_compiler_compile_options.cmake.
if(EXISTS ${CMAKE_TOOLCAHIN_DIR}/${ARCH}_compiler_compile_options.cmake)
    include(${CMAKE_TOOLCAHIN_DIR}/${ARCH}_compiler_compile_options.cmake)
endif()

message(STATUS TARGET_NAME: ${TARGET_NAME})
add_executable(${TARGET_NAME})
add_subdirectory(${SDK_BASE} ${SDK_OUT}/obj)

# The link options which are differ with toolchain should in ${ARCH}_compiler_link_options.cmake.
include(${CMAKE_TOOLCAHIN_DIR}/${ARCH}_compiler_link_options.cmake)

# Linker flags
target_link_options(
    ${TARGET_NAME}
    PUBLIC ${common_flags} ${linker_flags}
)

#Generate binary file from elf
compiler_generate_binary_output(${TARGET_NAME})

#Generate disassembly file from elf
compiler_generate_lst_output(${TARGET_NAME})

#Generate module memory size information
generate_module_info_output(${TARGET_NAME})
