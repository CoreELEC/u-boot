#! /bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

RTOS_BUILD_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)
BUILD_SYSTEM_DIR=$RTOS_BUILD_DIR/build_system

export RTOS_BUILD_DIR

# Parsing large package combination information
function package_target_verify() {

    ARCH_PREFIX=""
    PRODUCT_SUFFIX=""

    #Get the packed sequence selected by the user
    j=0
    while IFS= read -r LINE; do
        if [ $j == $CHOICE_PACKAGE ]; then
            PACKAGE_ARRY=($(echo $LINE | tr ' ' ' '))
        fi
        j=$((j + 1))
    done <$PACKAGE_COMBINATION

    #parameter check
    if [ -z "$PACKAGE_ARRY" ]; then
        echo -e "\033[41;33m package list is not set, please execute source scripts/pkg_env.sh \033[0m"
        exit 1
    fi

    #parameter check
    j=0
    for ((loop = 0; loop < ${#PACKAGE_ARRY[@]}; loop += 4)); do
        pkg_arch[j]=${PACKAGE_ARRY[loop]}
        pkg_soc[j]=${PACKAGE_ARRY[loop + 1]}
        pkg_board[j]=${PACKAGE_ARRY[loop + 2]}
        pkg_product[j]=${PACKAGE_ARRY[loop + 3]}
        ARCH_PREFIX="$ARCH_PREFIX""${pkg_arch[j]}""-"
        PRODUCT_SUFFIX="$PRODUCT_SUFFIX""-""${pkg_product[j]}"
        j+=1
    done

    # set the board configuration path
    IMAGE_BOARD_CONFIG_DIR="$RTOS_BUILD_DIR/image_packer/${pkg_soc[0]}/"

    # mirror storage path
    AML_IMAGE_STORAGE_PATH=$RTOS_BUILD_DIR/output/packages/"${ARCH_PREFIX}""${pkg_soc[0]}"-"${pkg_board[0]}"

    # create mirror file directory
    rm -fr $AML_IMAGE_STORAGE_PATH
    mkdir -p $AML_IMAGE_STORAGE_PATH
}

function compile_rtos_for_arm() {
    # target file path
    OUTPUT_PATH=${RTOS_BUILD_DIR}/output/$1-$3-$4

    IMAGE_PATH=${OUTPUT_PATH}/images
    BINARY_FILE=${IMAGE_PATH}/${KERNEL}-signed.bin
    DEBUG_FILE_PREFIX=${OUTPUT_PATH}/${KERNEL}/${KERNEL}
    XIP_CONFIG_FILE=${RTOS_BUILD_DIR}/boards/$1/$3/lscript.h
    BUILD_LINK_FILE=${RTOS_BUILD_DIR}/boards/$1/$3/lscript.h

    # Clean up rtos compilation intermediate files
    rm -rf $OUTPUT_PATH

    # rtos load address
    LINE=$(grep -m 1 "configTEXT_BASE" $BUILD_LINK_FILE)
    RTOS_LOAD_ADDR=$(echo "$LINE" | grep -oP '0x[0-9a-fA-F]+')

    # determine whether to enable the xip function
    RTOS_XIP=$(grep -E "^#define\s+CONFIG_XIP\s+[01]$" "$XIP_CONFIG_FILE" | awk '{print $3}')

    # start compile flow
    pushd $RTOS_BUILD_DIR

    source scripts/env.sh $1 $2 $3 $4

    if [ "$BACKTRACE_ENABLE" = "1" ]; then
        make backtrace
    else
        make
    fi

    if [ $? -ne 0 ]; then
        echo "bulid rtos image faile error:$?"
        popd
        exit 1
    fi

    if [ "$RTOS_XIP" = "1" ]; then
        make -f ${BUILD_SYSTEM_DIR}/xip.mk xip
        cp ${OUTPUT_PATH}/${KERNEL}/${KERNEL}.bin ${BINARY_FILE}
    fi

    mkimage -A ${ARCH} -O u-boot -T standalone -C none -a ${RTOS_LOAD_ADDR} -e ${RTOS_LOAD_ADDR} -n rtos -d ${BINARY_FILE} ${IMAGE_PATH}/rtos-uImage

    if [ "$RTOS_XIP" = "1" ]; then
        cp ${OUTPUT_PATH}/freertos/freertos_b.bin ${IMAGE_PATH}/rtos-xipA
        cp ${IMAGE_PATH}/* $AML_IMAGE_STORAGE_PATH/
    else
        test -f ${IMAGE_PATH}/rtos-uImage && cp ${IMAGE_PATH}/rtos-uImage $AML_IMAGE_STORAGE_PATH/rtos-uImage
    fi

    test -f ${DEBUG_FILE_PREFIX}.lst && cp ${DEBUG_FILE_PREFIX}.lst $AML_IMAGE_STORAGE_PATH/$1-$3.lst
    test -f ${DEBUG_FILE_PREFIX}.map && cp ${DEBUG_FILE_PREFIX}.map $AML_IMAGE_STORAGE_PATH/$1-$3.map

    popd
}

function compile_rtos_for_other() {
    # target file path
    OUTPUT_PATH=${RTOS_BUILD_DIR}/output/$1-$3-$4

    IMAGE_PATH=${OUTPUT_PATH}/images
    BINARY_FILE=${IMAGE_PATH}/${KERNEL}-signed.bin
    DEBUG_FILE_PREFIX=${OUTPUT_PATH}/${KERNEL}/${KERNEL}

    # Clean up rtos compilation intermediate files
    rm -rf $OUTPUT_PATH

    # start compile flow
    pushd $RTOS_BUILD_DIR

    source scripts/env.sh $1 $2 $3 $4

    make

    if [ "$1" == "xtensa" ]; then
        test -f ${BINARY_FILE} && cp ${BINARY_FILE} $AML_IMAGE_STORAGE_PATH/dspboot.bin
    else
        test -f ${BINARY_FILE} && cp ${BINARY_FILE} $AML_IMAGE_STORAGE_PATH/${KERNEL}-signed.bin
    fi

    test -f ${DEBUG_FILE_PREFIX}.lst && cp ${DEBUG_FILE_PREFIX}.lst $AML_IMAGE_STORAGE_PATH/$1-$3.lst
    test -f ${DEBUG_FILE_PREFIX}.map && cp ${DEBUG_FILE_PREFIX}.map $AML_IMAGE_STORAGE_PATH/$1-$3.map

    popd
}

# Compile rtos for all architectures
function compile_rtos_for_all() {
    for ((loop = 0; loop < ${#pkg_board[@]}; loop += 1)); do
        if [[ ${pkg_arch[loop]} == *"arm"* ]]; then
            compile_rtos_for_arm ${pkg_arch[loop]} ${pkg_soc[loop]} ${pkg_board[loop]} ${pkg_product[loop]}
        else
            compile_rtos_for_other ${pkg_arch[loop]} ${pkg_soc[loop]} ${pkg_board[loop]} ${pkg_product[loop]}
        fi
    done
}

# Compile the bootloader
function build_bootloader() {
    echo "start compiling bootloader ..."
    echo "<-------------- ${pkg_arch[0]} ${pkg_soc[0]} ${pkg_board[0]} ${pkg_product[0]} -------------->"

    #Select the compile parameters of the bootstrap
    case ${pkg_board[0]} in
    'ad401_a113l')
        uboot_type="a1_ad401_nand_rtos"
        ;;
    'ad403_a113l')
        uboot_type="a1_ad403_nand_rtos"
        ;;
    *) ;;
    esac

    if [ -z "$uboot_type" ]; then
        echo "Waring: Select board(${pkg_board[0]}) not support compile uboot"
        exit 1
    else
        pushd $RTOS_BUILD_DIR/boot/u-boot
        ./mk $uboot_type
        test -f build/u-boot.bin && cp -av build/u-boot.bin* $AML_IMAGE_STORAGE_PATH
        popd
    fi
}

function aml_image_package() {
    install $IMAGE_BOARD_CONFIG_DIR/platform.conf $AML_IMAGE_STORAGE_PATH/
    install $IMAGE_BOARD_CONFIG_DIR/usb_flow.aml $AML_IMAGE_STORAGE_PATH/
    install $IMAGE_BOARD_CONFIG_DIR/aml_sdc_burn.ini $AML_IMAGE_STORAGE_PATH/

    if [ -e "$AML_IMAGE_STORAGE_PATH/dspboot.bin" ]; then
        cp $IMAGE_BOARD_CONFIG_DIR/aml_upgrade_package.conf $AML_IMAGE_STORAGE_PATH/aml_upgrade_package.conf
    elif [ -e "$AML_IMAGE_STORAGE_PATH/rtos-xipA.bin" ]; then
        cp $IMAGE_BOARD_CONFIG_DIR/aml_upgrade_package_xip.conf $AML_IMAGE_STORAGE_PATH/aml_upgrade_package.conf
    else
        cp $IMAGE_BOARD_CONFIG_DIR/aml_upgrade_package_ndsp.conf $AML_IMAGE_STORAGE_PATH/aml_upgrade_package.conf
    fi

    $RTOS_BUILD_DIR/image_packer/aml_image_v2_packer -r $AML_IMAGE_STORAGE_PATH/aml_upgrade_package.conf $AML_IMAGE_STORAGE_PATH $AML_IMAGE_STORAGE_PATH/aml_upgrade_package.img

    cd $AML_IMAGE_STORAGE_PATH && rm $(ls | grep -v ".lst" | grep -v ".map" | grep -v ".img")
}

package_target_verify
compile_rtos_for_all
build_bootloader
aml_image_package
