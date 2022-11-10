#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#RTOS root directory
RTOS_BASE_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)

## external resource path ##
BL22_DIR=$RTOS_BASE_DIR/bl22
UBOOT_DIR=$RTOS_BASE_DIR/boot
LZ4_DIR=$RTOS_BASE_DIR/lib/utilities/lz4

#Get the current project environment variables
source $RTOS_BASE_DIR/scripts/env.sh arm64 c3 aw419_c308l fastboot

#RTOS object file path
RTOS_BUILD_DIR=$RTOS_BASE_DIR/output/$ARCH-$BOARD-$PRODUCT/freertos
RTOS_IMAGE_A=$RTOS_BUILD_DIR/rtos_1.bin
RTOS_IMAGE_B=$RTOS_BUILD_DIR/rtos_2.bin

function bl22_compile() {
	if [ -d $BL22_DIR ]; then
		pushd $BL22_DIR
		./mk c3
		cp ./bl22.bin $RTOS_BUILD_DIR/bl22.bin
		popd
	fi
}

function lz4_rtos() {
	if [ -d $LZ4_DIR ]; then
		pushd $LZ4_DIR
		cp $RTOS_IMAGE_A .
		./self_decompress_tool.sh -a ./self_decompress_head.bin -b ./rtos_1.bin -l 0x04c00000 -j 0x04e00000 -d 0
		cp ./self_decompress_firmware.bin $RTOS_IMAGE_A
		popd
	fi
}

function package_fastboot() {
	if [ -d $UBOOT_DIR ]; then
		if [ -d $UBOOT_DIR/fastboot ]; then
			rm -rf $UBOOT_DIR/fastboot
		fi
		mkdir -p $UBOOT_DIR/fastboot
	fi
	pushd $UBOOT_DIR
	cp $RTOS_IMAGE_A $UBOOT_DIR/fastboot
	cp $RTOS_IMAGE_B $UBOOT_DIR/fastboot
	cp $RTOS_BUILD_DIR/bl22.bin $UBOOT_DIR/fastboot
	#./mk c3_aw419 --update-bl2 --bl31 ./blob-bl31.bin.signed
	./mk c3_aw419 --update-bl2 --update-bl2e --bl31 ./blob-bl31.bin.signed
	popd
}

#compile the rtos image
cd $RTOS_BASE_DIR && make scatter
#lz4 compression
lz4_rtos
#compile the bl22 image
bl22_compile
#compile the u-boot image
package_fastboot
