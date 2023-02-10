#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#RTOS root directory
RTOS_BASE_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)

#Board Mapping Combination
BOARD_DEFINE_REF=(c3_aw409 c3_aw402 c3_aw419)
BOARD_DEFINE_PAR=(aw409_c302x aw402_c302x aw419_c308l)

## external resource path ##
if [ -z $1 ] || [ -z $2 ] || [ -z $3 ]; then
	echo -e "\033[41;33m Notice: parameter error !!! \033[0m"
	echo -e "\033[33m usage: ./c3_fastboot.sh bl22_path u-boot_path board_type (optional:load_address)\033[0m"
	exit 1
else
	BL22_DIR=$1
	UBOOT_DIR=$2
	BOARD_TYPE=$3
	if [ $4 ] && [[ "$4" =~ ^0x.* ]]; then
		RTOS_TARGET_ADDRESS=$4
	fi
fi

#Parse the specified hardware type
for ((i = 0; i < ${#BOARD_DEFINE_PAR[@]}; i++)); do
	if [ ${BOARD_DEFINE_PAR[i]} == $BOARD_TYPE ]; then
		BOARD_TYPE_MAPPING=${BOARD_DEFINE_REF[i]}
		break
	fi
done

#parameter check
if [ -z $BOARD_TYPE_MAPPING ]; then
	echo -e "\033[41;33m Notice: parameter error !!! \033[0m"
	echo -e "\033[33m board_type: aw409_c302x / aw402_c302x / aw419_c308l\033[0m"
	exit 1
fi

#Get the rtos target address (The configuration needs to be consistent with the lscript.h file)
if [ -z $RTOS_TARGET_ADDRESS ]; then
	case $BOARD_TYPE_MAPPING in
	'c3_aw409')
		RTOS_TARGET_ADDRESS=0x5400000
		;;
	'c3_aw402')
		RTOS_TARGET_ADDRESS=0x7600000
		;;
	*)
		RTOS_TARGET_ADDRESS=0x9000000
		;;
	esac
fi

#Get the loading address of rtos2
let "RTOS2_TARGET_ADDRESS=$RTOS_TARGET_ADDRESS+0x200000"
RTOS2_TARGET_ADDRESS=$(printf '0x%x\n' $RTOS2_TARGET_ADDRESS)

#Clear cache files
[ -d $RTOS_BASE_DIR/output ] && rm -rf $RTOS_BASE_DIR/output

#Get the current project environment variables
source $RTOS_BASE_DIR/scripts/env.sh arm64 c3 $BOARD_TYPE fastboot

#RTOS object file path
RTOS_BUILD_DIR=$RTOS_BASE_DIR/output/$ARCH-$BOARD-$PRODUCT/freertos
RTOS_IMAGE_A=$RTOS_BUILD_DIR/rtos_1.bin
RTOS_IMAGE_B=$RTOS_BUILD_DIR/rtos_2.bin

function toolchain_prepare() {
	echo "<============ TOOLCHAIN INFO RTOS ============>"
	CROSSTOOL=$RTOS_BASE_DIR/arch/$ARCH/toolchain/$COMPILER*$TOOLCHAIN_KEYWORD
	rm -rf $RTOS_BASE_DIR/output/toolchains
	mkdir $RTOS_BASE_DIR/output/toolchains
	tar -xf $CROSSTOOL.tar.xz -C $RTOS_BASE_DIR/output/toolchains --strip-components=1
	ls -la $RTOS_BASE_DIR/output/toolchains/bin
	$RTOS_BASE_DIR/output/toolchains/bin/aarch64-none-elf-gcc -v
	echo "<============ TOOLCHAIN INFO RTOS ============>"
}

function rtos_config_prepare() {
	CONFIG_FILE=$RTOS_BASE_DIR/boards/$ARCH/$BOARD/lscript.h
	sed -i '/.*#define configTEXT_BASE*/c\#define configTEXT_BASE '${RTOS_TARGET_ADDRESS}'' $CONFIG_FILE
	sed -i '/.*#define CONFIG_SCATTER_LOAD_ADDRESS*/c\#define CONFIG_SCATTER_LOAD_ADDRESS '${RTOS2_TARGET_ADDRESS}'' $CONFIG_FILE
}

function lz4_rtos() {
	pushd $RTOS_BASE_DIR/lib/utilities/lz4
	cp $RTOS_IMAGE_A .
	#Get the rtos target address
	./self_decompress_tool.sh -a ./self_decompress_head.bin -b ./rtos_1.bin -l 0x04c00000 -j $RTOS_TARGET_ADDRESS -d 0 -s $RTOS2_TARGET_ADDRESS
	cp ./self_decompress_firmware.bin $RTOS_IMAGE_A
	popd
}

function bl22_compile() {
	if [ -d $BL22_DIR ]; then
		pushd $BL22_DIR
		if [ -f ./mk ]; then
			./mk c3 $BOARD_TYPE
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
	#./mk c3_aw419 --update-bl2 --update-bl2e --bl31 ./fip/blob-bl31.bin.signed
	./mk $BOARD_TYPE_MAPPING
	popd
}

function debug_info() {
	echo "<============ Kconfig RTOS ============>"
	cat $RTOS_BASE_DIR/Kconfig
	echo "<============ CMakeLists RTOS ============>"
	cat $RTOS_BASE_DIR/CMakeLists.txt
	echo "<============ XML RTOS ============>"
	cat $RTOS_BUILD_DIR/rtos_sdk_manifest.xml
	echo "<============ XML OLD RTOS ============>"
	cat $RTOS_BUILD_DIR/rtos_sdk_manifest_old.xml
	echo "<============ JENKINS FOR RTOS ============>"
}

#Configure the RTOS environment
if [ $4 ] && [[ "$4" =~ ^0x.* ]]; then
	rtos_config_prepare
fi
#Compile toolchain preparation
toolchain_prepare
#compile the rtos image
cd $RTOS_BASE_DIR && make scatter
#lz4 compression
lz4_rtos
#compile the bl22 image
bl22_compile
#compile the u-boot image
package_fastboot
#debug
debug_info
