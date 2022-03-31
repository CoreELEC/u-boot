#! /bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

RTOS_BUILD_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)
BOARD_ARRY=(ad401_a113l ad403_a113l)
BOARD_LIST_ARRY=($(find ${RTOS_BUILD_DIR}/boards -mindepth 2 -maxdepth 2 -type d ! -name ".*" | xargs basename -a | sort -n))

#Equipment validity check
function board_validity_check() {
    #parameter check
    if [ -z "$1" ]; then
        echo -e "\033[41;33m COMPILER is not set, Please execute source scripts/env.sh \033[0m"
        exit 0
    fi
    #Determine whether this board is supported
    for boards in ${BOARD_ARRY[@]}; do
        if [ "$boards" == "$1" ] || [[ "$1" =~ $boards"_hifi" ]]; then
            BUILD_RTOS=1
        fi
    done
    if [ -z "$BUILD_RTOS" ]; then
        echo -e "\033[41;33m WARNING: $1 boards do not support packaging!!!\033[0m"
        exit 0
    fi
    #Identify and determine the corresponding board combination
    for boards in ${BOARD_LIST_ARRY[@]}; do
        if [ $boards == $1 ] && [[ "$1" =~ "_hifi" ]]; then
            BOARD_NAME=${1%_hifi*}
            DSP_BOARD_NAME=$1
            BUILD_DSP=1
        elif [ $boards == $1 ]; then
            BOARD_NAME=$1
            for dsp in ${BOARD_LIST_ARRY[@]}; do
                if [[ "$dsp" =~ $BOARD_NAME"_hifi" ]]; then
                    DSP_BOARD_NAME=$dsp
                    BUILD_DSP=1
                fi
            done
        fi
    done
    #Set up the build project
    BUILD_CLEAN=1
    BUILD_IMAGE=1
    BUILD_UBOOT=1
    #Set the board configuration path
    IMAGE_BOARD_CONFIG_DIR="image_packer/$SOC/"
}

#select build board
function build_board_select() {

    #Determine dsp and arm types
    case ${SOC} in
    'a1')
        RTOS_BOARD_ARCH="arm64"
        DSP_BOARD_ARCH="xtensa"
        RTOS_BOARD_PRODUCT="speaker"
        DSP_BOARD_PRODUCT="hifi_dsp"
        ;;
    *)
        echo "Unsupported soc type:${SOC}"
        exit 0
        ;;
    esac

    #Determine the hardware model, boot type
    case $1 in
    'ad401_a113l')
        UBOOT_BOARDNAME="a1_ad401_nand_rtos"
        ;;
    'ad403_a113l')
        UBOOT_BOARDNAME="a1_ad403_nor_rtos"
        unset BUILD_DSP
        unset DSP_BOARD_ARCH
        ;;
    *)
        echo "Unsupported board type:$1"
        exit 0
        ;;
    esac

    #Arch prefix settings
    if [ -n "$RTOS_BOARD_ARCH" ]; then
        ARCH_PREFIX="${RTOS_BOARD_ARCH}""-"
    fi
    if [ -n "$DSP_BOARD_ARCH" ]; then
        ARCH_PREFIX="${ARCH_PREFIX}""${DSP_BOARD_ARCH}""-"
    fi
}

#build rtos dsp
function build_rtos_dsp() {

    pushd $RTOS_BUILD_DIR

    source scripts/env.sh ${DSP_BOARD_ARCH} ${SOC} ${DSP_BOARD_NAME} ${DSP_BOARD_PRODUCT}

    make

    test -f ${DSP_SDK_SINGED_BIN_FILE} && cp ${DSP_SDK_SINGED_BIN_FILE} $PROJECT_BUILD_OUT_IMAGE_PATH/dspboot.bin
    rm -rf $DSP_SDK_OUT_PATH

    popd
}

#build rtos uimage
function build_rtos_image() {

    pushd $RTOS_BUILD_DIR

    source scripts/env.sh ${RTOS_BOARD_ARCH} ${SOC} ${BOARD_NAME} ${RTOS_BOARD_PRODUCT}

    if [ -n "$1" ] &&
        [ $1 == "backtrace" ]; then
        make backtrace
    else
        make
    fi

    if [ $? -ne 0 ]; then
        echo "bulid rtos image faile error:$?"
        popd
        exit 1
    else
        mkimage -A ${ARCH} -O u-boot -T standalone -C none -a 0x1000 -e 0x1000 -n rtos -d ${RTOS_SDK_SINGED_BIN_FILE} ${RTOS_SDK_IMAGE_PATH}/rtos-uImage
        test -f ${RTOS_SDK_IMAGE_PATH}/rtos-uImage && cp ${RTOS_SDK_IMAGE_PATH}/rtos-uImage $PROJECT_BUILD_OUT_IMAGE_PATH/rtos-uImage
        rm -rf $RTOS_SDK_OUT_PATH
    fi

    popd
}

#build aml image
function build_aml_image() {
    install $RTOS_BUILD_DIR/$IMAGE_BOARD_CONFIG_DIR/platform.conf $PROJECT_BUILD_OUT_IMAGE_PATH/
    install $RTOS_BUILD_DIR/$IMAGE_BOARD_CONFIG_DIR/usb_flow.aml $PROJECT_BUILD_OUT_IMAGE_PATH/
    install $RTOS_BUILD_DIR/$IMAGE_BOARD_CONFIG_DIR/aml_sdc_burn.ini $PROJECT_BUILD_OUT_IMAGE_PATH/
    if [ -z "$BUILD_DSP" ]; then
        install $RTOS_BUILD_DIR/$IMAGE_BOARD_CONFIG_DIR/aml_upgrade_package_ndsp.conf $PROJECT_BUILD_OUT_IMAGE_PATH/
        $RTOS_BUILD_DIR/image_packer/aml_image_v2_packer -r $PROJECT_BUILD_OUT_IMAGE_PATH/aml_upgrade_package_ndsp.conf $PROJECT_BUILD_OUT_IMAGE_PATH $PROJECT_BUILD_OUT_IMAGE_PATH/aml_upgrade_package.img
    else
        install $RTOS_BUILD_DIR/$IMAGE_BOARD_CONFIG_DIR/aml_upgrade_package.conf $PROJECT_BUILD_OUT_IMAGE_PATH/
        $RTOS_BUILD_DIR/image_packer/aml_image_v2_packer -r $PROJECT_BUILD_OUT_IMAGE_PATH/aml_upgrade_package.conf $PROJECT_BUILD_OUT_IMAGE_PATH $PROJECT_BUILD_OUT_IMAGE_PATH/aml_upgrade_package.img
    fi
}

#build uboot
function build_uboot() {
    if [ $UBOOT_BOARDNAME == "none" ]; then
        echo "Select board($BOARD) not support compile uboot"
        exit 1
    else
        pushd $RTOS_BUILD_DIR/boot/
        ./mk $1
        test -f build/u-boot.bin && cp -av build/u-boot.bin* $PROJECT_BUILD_OUT_IMAGE_PATH
        popd
    fi
}

board_validity_check $BOARD
build_board_select $BOARD_NAME

export PROJECT_BUILD_OUT_IMAGE_PATH=${RTOS_BUILD_DIR}/output/package/"${ARCH_PREFIX}""${SOC}"-${BOARD_NAME}/images/

export RTOS_SDK_OUT_PATH=${RTOS_BUILD_DIR}/output/${RTOS_BOARD_ARCH}-${BOARD_NAME}-${RTOS_BOARD_PRODUCT}
export RTOS_SDK_IMAGE_PATH=${RTOS_BUILD_DIR}/output/${RTOS_BOARD_ARCH}-${BOARD_NAME}-${RTOS_BOARD_PRODUCT}/images
export RTOS_SDK_SINGED_BIN_FILE=${RTOS_BUILD_DIR}/output/${RTOS_BOARD_ARCH}-${BOARD_NAME}-${RTOS_BOARD_PRODUCT}/images/${KERNEL}-signed.bin

export DSP_SDK_OUT_PATH=${RTOS_BUILD_DIR}/output/${DSP_BOARD_ARCH}-${DSP_BOARD_NAME}-${DSP_BOARD_PRODUCT}
export DSP_SDK_IMAGE_PATH=${RTOS_BUILD_DIR}/output/${DSP_BOARD_ARCH}-${DSP_BOARD_NAME}-${DSP_BOARD_PRODUCT}/images
export DSP_SDK_SINGED_BIN_FILE=${RTOS_BUILD_DIR}/output/${DSP_BOARD_ARCH}-${DSP_BOARD_NAME}-${DSP_BOARD_PRODUCT}/images/${KERNEL}-signed.bin

test -n "$BUILD_CLEAN" && rm -fr $DSP_SDK_OUT_PATH
test -n "$BUILD_CLEAN" && rm -fr $RTOS_SDK_OUT_PATH
test -n "$BUILD_CLEAN" && rm -fr $PROJECT_BUILD_OUT_IMAGE_PATH
mkdir -p $PROJECT_BUILD_OUT_IMAGE_PATH

test -n "$BUILD_DSP" && build_rtos_dsp
test -n "$BUILD_RTOS" && build_rtos_image $1
test -n "$BUILD_UBOOT" && build_uboot $UBOOT_BOARDNAME
test -n "$BUILD_IMAGE" && build_aml_image

echo "======Done======"
echo "Image Path: $PROJECT_BUILD_OUT_IMAGE_PATH "
ls -la $PROJECT_BUILD_OUT_IMAGE_PATH
