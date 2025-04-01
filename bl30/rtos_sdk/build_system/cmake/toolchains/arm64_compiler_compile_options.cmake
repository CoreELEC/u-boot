# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(${COMPILER} STREQUAL "clang+llvm")
    if(CONFIG_LTO_OPTIMIZATION)
        # clang+llvm not support fat LTO and full option cause a coredump
        message(STATUS "LTO option enabled")
        set(LTO_OPTIONS "-flto=thin")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --config aarch64__nosys.cfg")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --config aarch64__nosys.cfg")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --config aarch64__nosys.cfg")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -MD -g -ffunction-sections -fdata-sections -march=armv8-a ${LTO_OPTIONS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -MD -g -ffunction-sections -fdata-sections -march=armv8-a ${LTO_OPTIONS}")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -O2 -MD -g -ffunction-sections -fdata-sections -D__ASM -march=armv8-a ${LTO_OPTIONS}")

    add_compile_options(-fcolor-diagnostics)
else()
    if(CONFIG_LTO_OPTIMIZATION)
        message(STATUS "LTO option enabled")
        set(LTO_OPTIONS "-flto=auto -ffat-lto-objects")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --specs=nosys.specs")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --specs=nosys.specs")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --specs=nosys.specs")
    if(CONFIG_LIB_MCUBOOT_BOOTLOADER)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -MD -g -ffunction-sections -fdata-sections -march=armv8-a ${LTO_OPTIONS} ${c_compile_flags}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -MD -g -ffunction-sections -fdata-sections -march=armv8-a ${LTO_OPTIONS} ${cpp_compile_flags}")
        set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Os -MD -g -ffunction-sections -fdata-sections -march=armv8-a -D__ASM ${LTO_OPTIONS}")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -MD -g -ffunction-sections -fdata-sections -march=armv8-a ${LTO_OPTIONS} ${c_compile_flags}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -MD -g -ffunction-sections -fdata-sections -march=armv8-a ${LTO_OPTIONS} ${cpp_compile_flags}")
        set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -O2 -MD -g -ffunction-sections -fdata-sections -march=armv8-a -D__ASM ${LTO_OPTIONS}")
    endif()

    add_compile_options(-fdiagnostics-color=always)
endif()