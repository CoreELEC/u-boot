#! /bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

RTOS_BUILD_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)

#Package target check
function package_target_verify() {

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
    for ((loop = 0; loop < ${#PACKAGE_ARRY[@]}; loop += 4)); do
        arch=${PACKAGE_ARRY[loop]}
        soc=${PACKAGE_ARRY[loop + 1]}
        board=${PACKAGE_ARRY[loop + 2]}
        product=${PACKAGE_ARRY[loop + 3]}

        echo $arch $soc $board $product

        case ${product} in
        'hifi_dsp')
            BUILD_DSP=1
            DSP_ARCH=$arch
            DSP_SOC=$soc
            DSP_BOARD=$board
            DSP_PRODUCT=$product
            ;;
        'speaker')
            BUILD_RTOS=1
            RTOS_ARCH=$arch
            RTOS_SOC=$soc
            RTOS_BOARD=$board
            RTOS_PRODUCT=$product
            ;;
        *)
            echo "Unsupported product type:${product}"
            exit 1
            ;;
        esac

    done

    #Set the board configuration path
    IMAGE_BOARD_CONFIG_DIR="image_packer/$RTOS_SOC/"

    #Set up the build project
    BUILD_CLEAN=1
    BUILD_IMAGE=1
    BUILD_UBOOT=1
}

#Packaging environment configuration
function package_env_config() {

    #Select the compile parameters of the bootstrap
    case $1 in
    'ad401_a113l')
        UBOOT_BOARDNAME="a1_ad401_nand_rtos"
        ;;
    'ad403_a113l')
        UBOOT_BOARDNAME="a1_ad403_nor_rtos"
        unset BUILD_DSP
        unset DSP_ARCH
        ;;
    *)
        echo "Unsupported board type:$1"
        exit 1
        ;;
    esac

    #Arch prefix settings
    if [ -n "$RTOS_ARCH" ]; then
        ARCH_PREFIX="${RTOS_ARCH}""-"
        PRODUCT_SUFFIX="-""${RTOS_PRODUCT}"
    fi
    if [ -n "$DSP_ARCH" ]; then
        ARCH_PREFIX="${ARCH_PREFIX}""${DSP_ARCH}""-"
        PRODUCT_SUFFIX="${PRODUCT_SUFFIX}""-""${DSP_PRODUCT}"
    fi
}

#build rtos dsp
function build_rtos_dsp() {

    pushd $RTOS_BUILD_DIR

    source scripts/env.sh ${DSP_ARCH} ${DSP_SOC} ${DSP_BOARD} ${DSP_PRODUCT}

    make

    test -f ${DSP_SDK_SINGED_BIN_FILE} && cp ${DSP_SDK_SINGED_BIN_FILE} $PROJECT_BUILD_OUT_IMAGE_PATH/dspboot.bin
    rm -rf $DSP_SDK_OUT_PATH

    popd
}

#build rtos uimage
function build_rtos_image() {

    pushd $RTOS_BUILD_DIR

    source scripts/env.sh ${RTOS_ARCH} ${RTOS_SOC} ${RTOS_BOARD} ${RTOS_PRODUCT}

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

    test -f ${PROJECT_BUILD_OUT_IMAGE_PATH}/aml_upgrade_package.img && cp ${PROJECT_BUILD_OUT_IMAGE_PATH}/aml_upgrade_package.img \
        $PROJECT_BUILD_ALL_IMAGE_PATH/"${ARCH_PREFIX}"${RTOS_BOARD}"${PRODUCT_SUFFIX}".img
}

#build uboot
function build_uboot() {
    if [ -z "$UBOOT_BOARDNAME" ]; then
        echo "Select board($BOARD) not support compile uboot"
        exit 1
    else
        pushd $RTOS_BUILD_DIR/boot/
        ./mk $UBOOT_BOARDNAME
        test -f build/u-boot.bin && cp -av build/u-boot.bin* $PROJECT_BUILD_OUT_IMAGE_PATH
        popd
    fi
}

package_target_verify
package_env_config $RTOS_BOARD

export PROJECT_BUILD_ALL_IMAGE_PATH=${RTOS_BUILD_DIR}/output/package/images/
export PROJECT_BUILD_OUT_IMAGE_PATH=${RTOS_BUILD_DIR}/output/package/"${ARCH_PREFIX}""${RTOS_SOC}"-${RTOS_BOARD}/images/

export RTOS_SDK_OUT_PATH=${RTOS_BUILD_DIR}/output/${RTOS_ARCH}-${RTOS_BOARD}-${RTOS_PRODUCT}
export RTOS_SDK_IMAGE_PATH=${RTOS_BUILD_DIR}/output/${RTOS_ARCH}-${RTOS_BOARD}-${RTOS_PRODUCT}/images
export RTOS_SDK_SINGED_BIN_FILE=${RTOS_BUILD_DIR}/output/${RTOS_ARCH}-${RTOS_BOARD}-${RTOS_PRODUCT}/images/${KERNEL}-signed.bin

export DSP_SDK_OUT_PATH=${RTOS_BUILD_DIR}/output/${DSP_ARCH}-${DSP_BOARD}-${DSP_PRODUCT}
export DSP_SDK_IMAGE_PATH=${RTOS_BUILD_DIR}/output/${DSP_ARCH}-${DSP_BOARD}-${DSP_PRODUCT}/images
export DSP_SDK_SINGED_BIN_FILE=${RTOS_BUILD_DIR}/output/${DSP_ARCH}-${DSP_BOARD}-${DSP_PRODUCT}/images/${KERNEL}-signed.bin

[ ! -d "$PROJECT_BUILD_ALL_IMAGE_PATH" ] && mkdir -p $PROJECT_BUILD_ALL_IMAGE_PATH

test -n "$BUILD_CLEAN" && rm -fr $DSP_SDK_OUT_PATH
test -n "$BUILD_CLEAN" && rm -fr $RTOS_SDK_OUT_PATH
test -n "$BUILD_CLEAN" && rm -fr $PROJECT_BUILD_OUT_IMAGE_PATH
mkdir -p $PROJECT_BUILD_OUT_IMAGE_PATH

test -n "$BUILD_RTOS" && build_rtos_image $1
test -n "$BUILD_DSP" && build_rtos_dsp
test -n "$BUILD_UBOOT" && build_uboot
test -n "$BUILD_IMAGE" && build_aml_image

echo "======Done======"
echo "Image Path: $PROJECT_BUILD_OUT_IMAGE_PATH "
ls -la $PROJECT_BUILD_OUT_IMAGE_PATH
