# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(${COMPILER} STREQUAL "clang+llvm")
    if(CONFIG_LIBC_STD AND CONFIG_LIBC_AML)
        set(SYSTEM_LIBS ${SYSTEM_LIBS} m c nosys)
    elseif(CONFIG_LIBC_AML)
        set(SYSTEM_LIBS ${SYSTEM_LIBS} m)
    elseif(CONFIG_LIBC_STD)
        set(SYSTEM_LIBS ${SYSTEM_LIBS} m c)
    else()
        set(SYSTEM_LIBS ${SYSTEM_LIBS} m)
    endif()
else()
    #compiler gcc
    if(CONFIG_LIBC_STD AND CONFIG_LIBC_AML)
        set(SYSTEM_LIBS ${SYSTEM_LIBS} m c gcc nosys)
    elseif(CONFIG_LIBC_AML)
        set(SYSTEM_LIBS ${SYSTEM_LIBS} gcc m)
    elseif(CONFIG_LIBC_STD)
        set(SYSTEM_LIBS ${SYSTEM_LIBS} m c gcc)
    else()
        set(SYSTEM_LIBS ${SYSTEM_LIBS} m gcc)
    endif()

    if("${COLLECT_IS_CPP_LIBRARIES}" STREQUAL "1")
        message(STATUS "link c++ stand library for C++ use lib")
        set(SYSTEM_LIBS ${SYSTEM_LIBS} stdc++ c g gcc nosys)
    endif()

    if(CONFIG_LTO_OPTIMIZATION)
        message(STATUS "gcc LTO optimization enabled")
        set(LTO_SYMBOL_WRAP -u__wrap__malloc_r -u__wrap__free_r -u__wrap__realloc_r -u__wrap__calloc_r
        -u__wrap__getpid_r -u__wrap__kill_r -u_isatty -u_fstat)
    endif()

    if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 12)
        target_link_options(
            ${TARGET_NAME}
            PUBLIC "-Wl,--no-warn-rwx-segments"
        )
    endif()
endif()

if(CONFIG_LIB_MCUBOOT_BOOTLOADER)
    add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD COMMAND ${CMAKE_C_COMPILER} -I ${BOARD_DIR} -I ${PROJECT_BINARY_DIR} -DCONFIG_EXCEPTION_SVC=1 -DCONFIG_LIB_MCUBOOT_BOOTLOADER=1 -E -xc -P ${SDK_BASE}/boards/${ARCH}/lscript > ${BOARD_DIR}/lscript.ld)
else()
    add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD COMMAND ${CMAKE_C_COMPILER} -I ${BOARD_DIR} -I ${PROJECT_BINARY_DIR} -DCONFIG_EXCEPTION_SVC=1 -E -xc -P ${SDK_BASE}/boards/${ARCH}/lscript > ${BOARD_DIR}/lscript.ld)
endif()

target_link_libraries(
    ${TARGET_NAME}
    -Wl,--start-group
    ${LTO_SYMBOL_WRAP}
    ${COLLECT_LINK_OBJS}
    ${SYSTEM_LIBS} ${COLLECT_LINK_LIBRARIES}
    -Wl,--end-group
    -T"${BOARD_DIR}/lscript.ld"
)