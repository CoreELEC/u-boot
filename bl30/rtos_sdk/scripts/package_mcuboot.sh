#! /bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

MAIN_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)

#$1: bl2.bin
#$2: bl2_tmp.bin
#$3: bl2_fixed.bin
#$4: acs.bin
#$5: acs_fixed.bin
#$6: bl2_new.bin
#This function fix bl2.bin to bl2_fixed.bin, acs.bin to acs_fixed.bin,
#and then combine bl2_fixed.bin and acs_fixed.bin to bl2_new.bin
function fix_bl2() {

    declare -i blx_bin_limit=57344
    declare -i blx_acs_limit=4096
    declare -i blx_size=0
    declare -i remain_size=0

    blx_size=$((`stat -c %s $1`))
    if [ $blx_size -gt $blx_bin_limit ]; then
		echo "Error: ($1) too big. $blx_size > $blx_bin_limit"
		exit 1
	fi

    #fix bl2 to 56KB with zero
    remain_size=$((blx_bin_limit - blx_size))
    dd if=/dev/zero of=$2 bs=1 count=$remain_size
    cat $1 $2 > $3 2> /dev/zero && rm $2

    blx_size=$((`stat -c %s $4`))
    if [ "$blx_size" -gt "$blx_acs_limit" ]; then
        echo "Error: ($4) too big. $blx_size > $blx_acs_limit"
		exit 1
    fi
    #fix acs to 4KB with zero
    remain_size=$((blx_acs_limit - blx_size))
    dd if=/dev/zero of=$2 bs=1 count=$remain_size
    cat $4 $2 > $5 2> /dev/zero && rm $2

    #combine bl2.bin and acs.bin to bl2_new.bin
    cat $3 $5 > $6 2> /dev/zero && rm $3 $5

}

function encrypt() {
    local ret=0
    $ENCRYPT_TOOL $@
    ret=$?
    if [ 0 -ne "$ret" ]; then
        echo "Encrypt error: $ret"
        exit 1
    fi
}
#$1 mcuboot output directory
function encrypt_bootloader() {
    encrypt --bl3sig --input $BL31_IMG --output $BL31_IMG.enc --level v3 --type bl31
    encrypt --bl3sig --input $BL33_BIN --output $BL33_BIN.enc --level v3 --type bl33
    encrypt --bl2sig --input $BL2_NEW_BIN --output $BL2_ENCRYPT_BIN && rm $BL2_NEW_BIN
    encrypt --bootmk --output $1/mcuboot.bin \
        --bl2 $BL2_ENCRYPT_BIN \
        --bl31 $BL31_IMG.enc \
        --bl33 $BL33_BIN.enc --level v3 \
        --ddrfw1 $DDRFW_1 \
        --ddrfw2 $DDRFW_2 \
        --ddrfw3 $DDRFW_3 \
        --ddrfw4 $DDRFW_4 \
        --ddrfw5 $DDRFW_5 \
        && rm $BL2_ENCRYPT_BIN $BL31_IMG.enc $BL33_BIN.enc
}

#$1 soc
#$2 product
#$3 input bl33.bin's and output mcuboot.bin's directory: output/arch-board-product/images/
function package_mcuboot() {
   #define firmware parameters
	FIRMWARE_DIR=$MAIN_DIR/products/$2/build/$1

	ACS=$FIRMWARE_DIR/acs.bin
	BL2_BIN=$FIRMWARE_DIR/bl2.bin
	BL2_NEW_BIN=$FIRMWARE_DIR/bl2_new.bin
	BL2_ENCRYPT_BIN=$FIRMWARE_DIR/bl2.n.bin.sig
	BL31_BIN=$FIRMWARE_DIR/bl31.bin
	BL31_IMG=$FIRMWARE_DIR/bl31.img
	BL33_BIN=$3/bl33.bin
	DDRFW_1=$FIRMWARE_DIR/ddr4_1d.fw
	DDRFW_2=$FIRMWARE_DIR/ddr4_2d.fw
	DDRFW_3=$FIRMWARE_DIR/ddr3_1d.fw
	DDRFW_4=$FIRMWARE_DIR/piei.fw
	DDRFW_5=$FIRMWARE_DIR/aml_ddr.fw
	ENCRYPT_TOOL=$FIRMWARE_DIR/aml_encrypt_a1

    fix_bl2 \
        $BL2_BIN \
        $FIRMWARE_DIR/bl2_tmp.bin \
        $FIRMWARE_DIR/bl2_fixed.bin \
        $ACS \
        $FIRMWARE_DIR/acs_fix.bin \
        $BL2_NEW_BIN
    encrypt_bootloader $3
}

#$1 arch
#$2 soc
#$3 board
#$4 product
function compile_mcuboot() {
    # target file path
	export BUILD_MCUBOOT=1
    MCUBOOT_ARCH=$1
    MCUBOOT_SOC=$2
    MCUBOOT_BOARD=$3
    MCUBOOT_PRODUCT=$4
    IMAGE_STORAGE_PATH=$5

    MCUBOOT_OUTPUT_PATH=${MAIN_DIR}/output/$MCUBOOT_ARCH-$MCUBOOT_BOARD-$MCUBOOT_PRODUCT

    MCUBOOT_IMAGE_PATH=${MCUBOOT_OUTPUT_PATH}/images
    ORIGINAL_BINARY_FILE=${MCUBOOT_IMAGE_PATH}/bl33.bin
    DEBUG_FILE_PREFIX=${MCUBOOT_OUTPUT_PATH}/${KERNEL}/${KERNEL}

    # Clean up mcuboot compilation intermediate files
    rm -rf $MCUBOOT_OUTPUT_PATH

    # start compile flow
    pushd $MAIN_DIR

    source scripts/env.sh $MCUBOOT_ARCH $MCUBOOT_SOC $MCUBOOT_BOARD $MCUBOOT_PRODUCT

    if [ "$BACKTRACE_ENABLE" = "1" ]; then
        make backtrace
    else
        make
    fi

    if [ $? -ne 0 ]; then
        echo "Compile MCUBoot failed:$?"
        popd
        exit 1
    fi

    if [ -f "$MCUBOOT_IMAGE_PATH/$KERNEL-signed.bin" ]; then
        mv $MCUBOOT_IMAGE_PATH/$KERNEL-signed.bin $ORIGINAL_BINARY_FILE
    else
        echo "$MCUBOOT_OUTPUT_PATH/$KERNEL/$KERNEL.bin does not exist!"
        popd
        exit 1
    fi

    #package mcuboot
    package_mcuboot $MCUBOOT_SOC $MCUBOOT_PRODUCT $MCUBOOT_IMAGE_PATH
    test -f $MCUBOOT_IMAGE_PATH/mcuboot.bin && \
    cp -av $MCUBOOT_IMAGE_PATH/mcuboot.* $IMAGE_STORAGE_PATH/
    popd
}
