# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

set(SYSTEM_LIBS ${SYSTEM_LIBS} m gcc)

if("${COLLECT_IS_CPP_LIBRARIES}" STREQUAL "1")
    message(STATUS "link c++ stand library for C++ use lib")
    set(SYSTEM_LIBS ${SYSTEM_LIBS} stdc++ c)
endif()

#arch XTENSA not support LTO option
add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD COMMAND ${CMAKE_C_COMPILER} -I ${BOARD_DIR} -E -xc -P ${BOARD_DIR}/lsp_dsp/memmap.ld > ${BOARD_DIR}/lsp_dsp/memmap.xmm)
add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD COMMAND ${CMAKE_LDGEN_COMPILER} -b ${BOARD_DIR}/lsp_dsp/ >/dev/null)
target_link_libraries(
    ${TARGET_NAME}
    -Wl,--start-group
    ${COLLECT_LINK_OBJS}
    ${SYSTEM_LIBS} ${COLLECT_LINK_LIBRARIES}
    -Wl,--end-group
    -mlsp="${BOARD_DIR}/lsp_dsp/"
)