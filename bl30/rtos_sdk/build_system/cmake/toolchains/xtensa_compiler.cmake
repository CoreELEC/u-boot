# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

include("${CMAKE_CURRENT_LIST_DIR}/find_compiler.cmake")

set(CMAKE_SYSTEM_NAME Generic)

# Find GCC for XTENSA.
aml_find_compiler(COMPILER_GCC_CC xt-xcc)
aml_find_compiler(COMPILER_GCC_CXX xt-xc++)
aml_find_compiler(COMPILER_GCC_ASM xt-xcc)

# Find CLANG for XTENSA.
aml_find_compiler(COMPILER_CLANG_CC xt-clang)
aml_find_compiler(COMPILER_CLANG_CXX xt-clang++)
aml_find_compiler(COMPILER_CLANG_ASM xt-clang)

aml_find_compiler(COMPILER_NM xt-nm)
aml_find_compiler(COMPILER_OBJCOPY xt-objcopy)
aml_find_compiler(COMPILER_OBJDUMP xt-objdump)
aml_find_compiler(COMPILER_LD_GEN xt-genldscripts)

# Specify the cross compiler.
set(CMAKE_C_COMPILER ${COMPILER_GCC_CC} CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER ${COMPILER_GCC_CXX} CACHE FILEPATH "C++ compiler")
set(CMAKE_ASM_COMPILER ${COMPILER_GCC_ASM} CACHE FILEPATH "ASM compiler")
set(CMAKE_NM_COMPILER ${COMPILER_NM} CACHE FILEPATH "nm compiler")
set(CMAKE_OBJCOPY_COMPILER ${COMPILER_OBJCOPY} CACHE FILEPATH "objcopy compiler")
set(CMAKE_OBJDUMP_COMPILER ${COMPILER_OBJDUMP} CACHE FILEPATH "objdump compiler")
set(CMAKE_LDGEN_COMPILER ${COMPILER_LD_GEN} CACHE FILEPATH "genldscripts compiler")

# Disable compiler checks.
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Add target system root to cmake find path.
get_filename_component(COMPILER_DIR "${COMPILER_CC}" DIRECTORY)
get_filename_component(CMAKE_FIND_ROOT_PATH "${COMPILER_DIR}" DIRECTORY)

# Helper variables to abstracts some common compiler flags.
set(COMPILER_NO_WARNINGS "-w" CACHE INTERNAL "")
