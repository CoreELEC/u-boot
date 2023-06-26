#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#RTOS root directory
RTOS_BASE_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)

function usage() {
    echo -e "\033[41;33m Notice: parameter error !!! \033[0m"
    echo -e "\033[33m usage: ./coredump_process.sh (elf_file_path) (log_file_path)\033[0m"
    exit 1
}

if [ -s "$1" ] && [ -s "$2" ]; then
    COREDUMP_LOG=$(readlink -f "$2")
    COREDUMP_BIN=$(dirname "$COREDUMP_LOG")/coredump.bin
    COREDUMP_ELF=$(readlink -f "$1")

    # Parse coredump log file.
    $RTOS_BASE_DIR/lib/utilities/coredump/scripts/coredump_serial_log_parser.py $COREDUMP_LOG $COREDUMP_BIN

    if [ $? -ne 0 ]; then
        echo "Failed to parse coredump file ! $COREDUMP_LOG"
        exit 1
    fi

    # Start gdbstub server.
    $RTOS_BASE_DIR/lib/utilities/coredump/scripts/coredump_gdbserver.py $COREDUMP_ELF $COREDUMP_BIN &

    if [ $? -ne 0 ]; then
        echo "Failed to start gdbstub service !"
        exit 1
    fi

    # Start gdbstub terminal
    pushd $RTOS_BASE_DIR/output/toolchains/gcc-aarch64-none-elf/bin
    aarch64-none-elf-gdb $COREDUMP_ELF
    popd

    # Clean up the abnormal exit problem of gdbstub
    pid=$(lsof -t -i :1234)

    if [ -n "$pid" ]; then
        kill $pid
    fi
else
    usage
fi
