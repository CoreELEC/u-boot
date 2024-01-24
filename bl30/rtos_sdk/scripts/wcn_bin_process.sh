#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#RTOS root directory
RTOS_BASE_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)

TOOL_CHAIN_PATH=$RTOS_BASE_DIR/output/toolchains/gcc-riscv-none/bin

OBJCOPY=$TOOL_CHAIN_PATH/riscv-none-embed-objcopy

FW_ELF=$RTOS_BASE_DIR/output/$ARCH-$BOARD-$PRODUCT/$KERNEL/freertos.elf
FW_HEX=$RTOS_BASE_DIR/output/$ARCH-$BOARD-$PRODUCT/$KERNEL/fw.hex
FW_BIN=$RTOS_BASE_DIR/output/$ARCH-$BOARD-$PRODUCT/$KERNEL/fwall.bin
TARGET=$RTOS_BASE_DIR/output/$ARCH-$BOARD-$PRODUCT/$KERNEL/wifi_fw_w1u.bin

target_size=$((240 * 1024))  # 240kb

$OBJCOPY --gap-fill=0x00 -O ihex $FW_ELF $FW_HEX

$OBJCOPY --gap-fill=0x00 --input-target=ihex --output-target=binary $FW_HEX $TARGET

current_size=$(stat -c %s "$TARGET")

bytes_to_append=$((target_size - current_size))

if [ "$bytes_to_append" -gt 0 ]; then
    dd if=/dev/zero bs="$bytes_to_append" count=1 >> "$TARGET"
    echo "File size has been extended to $target_size bytes."
else
    echo "File is already at or larger than the target size."
fi
