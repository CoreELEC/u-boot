# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(${COMPILER} STREQUAL "clang+llvm")
    add_compile_options(-fcolor-diagnostics)
else()
    add_compile_options(-fdiagnostics-color=always)
endif()