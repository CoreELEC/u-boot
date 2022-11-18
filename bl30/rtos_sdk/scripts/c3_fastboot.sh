#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#RTOS root directory
RTOS_BASE_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)

## external resource path ##
if [ -z $1 ] || [ -z $2 ]; then
	echo -e "\033[41;33m Notice: parameter error !!! \033[0m"
	echo -e "\033[33m usage: ./c3_fastboot.sh bl22_path u-boot_path \033[0m"
	exit 1
else
	BL22_DIR=$1
	UBOOT_DIR=$2
fi

#Clear cache files
[ -d $RTOS_BASE_DIR/output ] && rm -rf $RTOS_BASE_DIR/output
#Get the current project environment variables
source $RTOS_BASE_DIR/scripts/env.sh arm64 c3 aw419_c308l fastboot

#RTOS object file path
RTOS_BUILD_DIR=$RTOS_BASE_DIR/output/$ARCH-$BOARD-$PRODUCT/freertos
RTOS_IMAGE_A=$RTOS_BUILD_DIR/rtos_1.bin
RTOS_IMAGE_B=$RTOS_BUILD_DIR/rtos_2.bin

function lz4_rtos() {
	pushd $RTOS_BASE_DIR/lib/utilities/lz4
	cp $RTOS_IMAGE_A .
	./self_decompress_tool.sh -a ./self_decompress_head.bin -b ./rtos_1.bin -l 0x04c00000 -j 0x04e00000 -d 0
	cp ./self_decompress_firmware.bin $RTOS_IMAGE_A
	popd
}

function bl22_compile() {
	if [ -d $BL22_DIR ]; then
		pushd $BL22_DIR
		if [ -f ./mk ]; then
			echo aaaaaaa
			./mk c3
		fi
		cp ./bl22.bin $RTOS_BUILD_DIR/bl22.bin
		popd
	fi
}

function package_fastboot() {
	pushd $UBOOT_DIR
	if [ -d ./fastboot ]; then
		rm -rf ./fastboot
	fi
	mkdir -p ./fastboot
	cp $RTOS_IMAGE_A ./fastboot
	cp $RTOS_IMAGE_B ./fastboot
	cp $RTOS_BUILD_DIR/bl22.bin ./fastboot
	#./mk c3_aw419 --update-bl2 --bl31 ./blob-bl31.bin.signed
	#./mk c3_aw419 --update-bl2 --update-bl2e --bl31 ./blob-bl31.bin.signed
	./mk c3_aw419
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
