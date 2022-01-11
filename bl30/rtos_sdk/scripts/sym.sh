#!/bin/bash

# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

###############################################################
# Function: Generate symbol table and rebuild image.
###############################################################
STACK_TRACE_CONFIG_STR='CONFIG_STACK_TRACE=y'
STACK_TRACE_CONFIG_GEN='CONFIG_STACK_TRACE_SYM=y'
STACK_TRACE_EXPAND_CONFIG_STR='CONFIG_STACK_TRACE_EXPAND=y'
STACK_TRACE_KCONFIG_STR='\nconfig STACK_TRACE_SYM\n\tbool "stackmark"'
STACK_TRACE_CMAKE_FILE=${SDK_BASE}/libs/stack_trace/CMakeLists.txt
STACK_TRACE_CONFIG_FILE=${SDK_BASE}/products/${PRODUCT}/prj.conf
STACK_TRACE_KCONFIG_FILE=${SDK_BASE}/libs/stack_trace/Kconfig
STACK_TRACE_PROJECT_OUT_FILE=${kernel_BUILD_DIR}/.config
STACK_TRACE_BUILD_ELF_IMAGE_FILE=${kernel_BUILD_DIR}/freertos.elf

#generate symbol table and rebuild
rebuild_back_trace_image() {
    make -f ${SDK_BASE}/build/symtable.mk backtrace
    touch ${STACK_TRACE_CMAKE_FILE}
    echo -en ${STACK_TRACE_KCONFIG_STR} >>$STACK_TRACE_KCONFIG_FILE
    sed -i '/'${STACK_TRACE_CONFIG_STR}'/a '${STACK_TRACE_CONFIG_GEN}'' ${STACK_TRACE_CONFIG_FILE}
    make -f ${SDK_BASE}/Makefile
    sed -i '/^'${STACK_TRACE_CONFIG_GEN}'/d' ${STACK_TRACE_CONFIG_FILE}
    sed -i '/config STACK_TRACE_SYM/,'/stackmark'/d' $STACK_TRACE_KCONFIG_FILE
    truncate -s -1 $STACK_TRACE_KCONFIG_FILE
    rm ${SDK_BASE}/libs/stack_trace/system_map_addr.c ${SDK_BASE}/libs/stack_trace/system_map_sym.c
}

#elf file validity judgment
elf_file_validity_judgment() {
    currentTime=$(date +%s)
    elfFileTime=$(stat -c %Y $STACK_TRACE_BUILD_ELF_IMAGE_FILE)
    diffTime=$(expr $currentTime - $elfFileTime)
    if [ $diffTime -lt 2 ]; then
        echo "ready rebuild ${BOARD}-${PRODUCT} image"
        rebuild_back_trace_image
    fi
}

# #check need to recompile
if [ $(grep -c "$STACK_TRACE_EXPAND_CONFIG_STR" $STACK_TRACE_PROJECT_OUT_FILE) -ne '0' ] &&
    [ $(grep -c "$STACK_TRACE_CONFIG_GEN" $STACK_TRACE_CONFIG_FILE) -eq '0' ] &&
    [ -f "$STACK_TRACE_BUILD_ELF_IMAGE_FILE" ]; then
    elf_file_validity_judgment
    exit 0
fi
