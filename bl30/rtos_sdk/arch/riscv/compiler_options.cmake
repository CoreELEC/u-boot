# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

set(common_flags "")
set(c_flags "")

set(linker_flags "-Wl,--print-memory-usage,-Map=${TARGET_NAME}.map,--gc-sections")

if(CONFIG_LTO_OPTIMIZATION)
set(LTO_OPTIONS "-flto -ffat-lto-objects")
endif()

if(CONFIG_LIBC_STD)
set(linker_flags "${linker_flags},--wrap=_malloc_r,--wrap=_free_r,--wrap=_realloc_r,--wrap=_calloc_r")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -ffunction-sections -fdata-sections -fno-common -fgnu89-inline -march=rv32imc -mabi=ilp32 ${LTO_OPTIONS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --specs=nano.specs --specs=nosys.specs")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -ffunction-sections -fdata-sections -fno-common -fgnu89-inline -march=rv32imc -mabi=ilp32 ${LTO_OPTIONS} -nostdlib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --specs=nano.specs --specs=nosys.specs")
if(CONFIG_BACKTRACE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fno-optimize-sibling-calls")
endif()
if(CONFIG_STACK_PROTECTOR_STRONG)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
endif()

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -O2 -g -ffunction-sections -fdata-sections -fno-common -fgnu89-inline -march=rv32imc -mabi=ilp32 ${LTO_OPTIONS} -D__ASM")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --specs=nano.specs --specs=nosys.specs")
