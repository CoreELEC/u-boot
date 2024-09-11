#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#RTOS root directory
RTOS_BASE_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)

RTOS_TARGET_ADDRESS=0x2200000
BOARD_TYPE=aw402_c302x
SENSOR_TYPE=SC301IOT
DDR_SIZE=128m
UBOOT_CFG=c3_aw402

while [[ $# -gt 0 ]]; do
    case "$1" in
    -a | --address)
        RTOS_TARGET_ADDRESS=$2
        shift 2
        ;;
    -b | --board)
        BOARD_TYPE=$2
        shift 2
        ;;
    -s | --sensor)
        SENSOR_TYPE=$2
        if [[ "$2" =~ ^SENSOR=.* ]]; then
            SENSOR_TYPE=${2#SENSOR=}
        fi
        shift 2
        ;;
    --bl22)
        BL22_DIR=$2
        shift 2
        ;;
    --uboot)
        UBOOT_DIR=$2
        shift 2
        ;;
    --ipc-ddr-size)
        DDR_SIZE=$2
        shift 2
        ;;
    --ubootcfg)
        UBOOT_CFG=$2
        shift 2
        ;;
    -o | --out)
        INSTALL_DIR=$2
        shift 2
        ;;
    -h | --help)
        echo -e "\033[41;33m gen_fastboot.sh help info \033[0m"
        echo "Usage: fastboot [OPTIONS]"
        echo
        echo "Options:"
        echo "  -a/--address    et address to load (e.g., -a 0x2200000)"
        echo "  -b/--board      Set board type (e.g., -b aw402_c302x)"
        echo "  -s/--sensor     Set ipc sensor type (e.g., -s IMX290)"
        echo "  --b22 PATH      Set BL22 path"
        echo "  -h              Display this help information"
        echo
        echo "For more details, refer to the fastboot documentation."
        exit 1
        shift
        ;;
    --)
        shift
        break
        ;;
    -*|--*)
        echo "Invalid option: $1" >&2
        exit 1
        ;;
    *)
        echo "No more options, just arguments" >&2
        exit 1
        ;;
    esac
done

if [[ ! -d "$BL22_DIR" ]]; then
    echo "The provided bl22 path is not a directory: $BL22_DIR"
    exit 1
fi

#Retrieve the RTOS loading address; if none exists, set it to the default address.
if [ -z $RTOS_TARGET_ADDRESS ]; then
    RTOS_TARGET_ADDRESS=0x02200000
fi

#Calculate the RTOS2 loading address
let "RTOS2_TARGET_ADDRESS=$RTOS_TARGET_ADDRESS+0x100000"
RTOS2_TARGET_ADDRESS=$(printf '0x%x\n' $RTOS2_TARGET_ADDRESS)

#Clear cache files
[ -d $RTOS_BASE_DIR/output ] && rm -rf $RTOS_BASE_DIR/output

#Get the current project environment variables
source $RTOS_BASE_DIR/scripts/env.sh arm64 c3 $BOARD_TYPE fastboot
if [ "$?" -ne 0 ]; then
    echo "Error: Incorrect board type selection!"
    exit 1
fi

#IPC sensor model configuration.
if [ -z $SENSOR_TYPE ]; then
    case $BOARD_TYPE in
    'aw402_c302x')
        SENSOR_TYPE=SC301IOT
        ;;
    *)
        SENSOR_TYPE=IMX290
        ;;
    esac
fi

#RTOS object file path
RTOS_BUILD_DIR=$RTOS_BASE_DIR/output/$ARCH-$BOARD-$PRODUCT/freertos
RTOS_IMAGE_1=$RTOS_BUILD_DIR/rtos_1.bin
RTOS_IMAGE_2=$RTOS_BUILD_DIR/rtos_2.bin
BL22_IMAGE=$RTOS_BUILD_DIR/bl22.bin

function toolchain_prepare() {
    echo "<============ TOOLCHAIN INFO RTOS ============>"
    CROSSTOOL=$RTOS_BASE_DIR/arch/$ARCH/toolchain/$COMPILER*$TOOLCHAIN_KEYWORD
    TOOLCHAIN_DIR=$RTOS_BASE_DIR/output/toolchains/$COMPILER-$TOOLCHAIN_KEYWORD
    rm -rf $RTOS_BASE_DIR/output/toolchains
    mkdir -p $TOOLCHAIN_DIR
    tar -xf $CROSSTOOL.tar.xz -C $TOOLCHAIN_DIR --strip-components=1
    ls -la $TOOLCHAIN_DIR/bin
    $TOOLCHAIN_DIR/bin/aarch64-none-elf-gcc -v
    echo "<============ TOOLCHAIN INFO RTOS ============>"
}

#Configure the compile-time address.
function rtos_config_prepare() {
    CONFIG_FILE=$RTOS_BASE_DIR/boards/$ARCH/$BOARD/lscript.h
    sed -i '/.*#define configTEXT_BASE*/c\#define configTEXT_BASE '${RTOS_TARGET_ADDRESS}'' $CONFIG_FILE
    sed -i '/.*#define CONFIG_SCATTER_LOAD_ADDRESS*/c\#define CONFIG_SCATTER_LOAD_ADDRESS '${RTOS2_TARGET_ADDRESS}'' $CONFIG_FILE
    case $DDR_SIZE in
    '128m')
        sed -i '/.*#define configSRAM_START*/c\#define configSRAM_START '0x3000000'' $CONFIG_FILE
        sed -i '/.*#define configSRAM_LEN*/c\#define configSRAM_LEN '0x1700000'' $CONFIG_FILE
        sed -i '/.*#define configVENC_BASE*/c\#define configVENC_BASE '0x04700000'' $CONFIG_FILE
        sed -i '/.*#define configVENC_LEN*/c\#define configVENC_LEN '0x0B00000'' $CONFIG_FILE
        sed -i '/.*#define configISP_RGB_BASE*/c\#define configISP_RGB_BASE '0x6000000'' $CONFIG_FILE
        ;;
    '256m')
        sed -i '/.*#define configSRAM_START*/c\#define configSRAM_START '0x9B00000'' $CONFIG_FILE
        sed -i '/.*#define configSRAM_LEN*/c\#define configSRAM_LEN '0x3200000'' $CONFIG_FILE
        sed -i '/.*#define configVENC_BASE*/c\#define configVENC_BASE '0xCD00000'' $CONFIG_FILE
        sed -i '/.*#define configVENC_LEN*/c\#define configVENC_LEN '0x2A00000'' $CONFIG_FILE
        sed -i '/.*#define configISP_RGB_BASE*/c\#define configISP_RGB_BASE '0x3700000'' $CONFIG_FILE
        ;;
    *)
        sed -i '/.*#define configSRAM_START*/c\#define configSRAM_START '0x3000000'' $CONFIG_FILE
        sed -i '/.*#define configSRAM_LEN*/c\#define configSRAM_LEN '0x1700000'' $CONFIG_FILE
        sed -i '/.*#define configVENC_LEN*/c\#define configVENC_LEN '0x04700000'' $CONFIG_FILE
        sed -i '/.*#define configISP_RGB_BASE*/c\#define configISP_RGB_BASE '0x6000000'' $CONFIG_FILE
        ;;
    esac
}

function sensor_config_choose() {
	sed -i '/CONFIG_SENSOR_SC401AI/d' $RTOS_BASE_DIR/boards/$ARCH/$BOARD/defconfig
	sed -i '/CONFIG_SENSOR_SC5336P/d' $RTOS_BASE_DIR/boards/$ARCH/$BOARD/defconfig
	if [ "${SENSOR_TYPE}" == "SC401AI" ]; then
		sed -i '$ a CONFIG_SENSOR_SC401AI=y' $RTOS_BASE_DIR/boards/$ARCH/$BOARD/defconfig
	fi
	if [ "${SENSOR_TYPE}" == "SC5336P" ]; then
		sed -i '$ a CONFIG_SENSOR_SC5336P=y' $RTOS_BASE_DIR/boards/$ARCH/$BOARD/defconfig
	fi
}

function lz4_rtos() {
    pushd $RTOS_BASE_DIR/lib/utilities/lz4
    cp $RTOS_IMAGE_1 .
    #Get the rtos target address
    ./self_decompress_tool.sh -a ./self_decompress_head.bin -b ./rtos_1.bin -l 0x04c00000 -j $RTOS_TARGET_ADDRESS -d 0 -s $RTOS2_TARGET_ADDRESS
    cp ./self_decompress_firmware.bin $RTOS_IMAGE_1
    rm ./self_decompress_firmware.bin ./rtos_1.bin
    popd
}

function bl22_compile() {
    pushd $BL22_DIR
    if [ -f ./mk ]; then
        echo "./mk c3 $BOARD_TYPE $SENSOR_TYPE"
        ./mk c3 $BOARD_TYPE $SENSOR_TYPE
        if [ "$?" -ne 0 ]; then
            echo "RTOS-SDK: BL22 compilation failed !!!"
            exit 1
        fi
    fi
    cp ./bl22.bin $RTOS_BUILD_DIR/bl22.bin
    popd
}

function build_header() {
    local bl2_2=$1
    local rtos_1=$2
    local rtos_2=$3

    local size=0
    # 4KB for header (reserve for sign)
    local offset=4096
    local rem=0

    # build head for bl2-2.bin
    size=$(stat -c %s ${bl2_2})
    # insert uuid
    echo -n -e "\x6b\xe6\x1e\x1f\xac\xc0\x45\x12\x97\x3b\x32\x5f\x2e\x17\xa7\x2d" >${RTOS_BUILD_DIR}/_tmp_hdr.bin
    # insert size
    printf "%02x%02x%02x%02x" $(((size) & 0xff)) $((((size) >> 8) & 0xff)) \
        $((((size) >> 16) & 0xff)) $((((size) >> 24) & 0xff)) | xxd -r -ps >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    # insert offset
    printf "%02x%02x%02x%02x" $(((offset) & 0xff)) $((((offset) >> 8) & 0xff)) \
        $((((offset) >> 16) & 0xff)) $((((offset) >> 24) & 0xff)) | xxd -r -ps >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    # insert 8 bytes padding
    printf "\0\0\0\0\0\0\0\0" >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    # insert sha256
    openssl dgst -sha256 -binary ${bl2_2} >>${RTOS_BUILD_DIR}/_tmp_hdr.bin

    # build header for RTOS_1
    offset=$(expr ${offset} + ${size} + 4095)
    rem=$(expr ${offset} % 4096)
    offset=$(expr ${offset} - ${rem})
    size=$(stat -c %s ${rtos_1})
    echo -n -e "\x31\x9f\xb3\x9e\x1f\x9f\x48\x98\x96\x38\xca\xfa\x7e\xa4\x2c\xe9" >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    printf "%02x%02x%02x%02x" $(((size) & 0xff)) $((((size) >> 8) & 0xff)) \
        $((((size) >> 16) & 0xff)) $((((size) >> 24) & 0xff)) | xxd -r -ps >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    printf "%02x%02x%02x%02x" $(((offset) & 0xff)) $((((offset) >> 8) & 0xff)) \
        $((((offset) >> 16) & 0xff)) $((((offset) >> 24) & 0xff)) | xxd -r -ps >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    printf "\0\0\0\0\0\0\0\0" >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    openssl dgst -sha256 -binary ${rtos_1} >>${RTOS_BUILD_DIR}/_tmp_hdr.bin

    # build header for RTOS_2
    offset=$(expr ${offset} + ${size} + 4095)
    rem=$(expr ${offset} % 4096)
    offset=$(expr ${offset} - ${rem})
    size=$(stat -c %s ${rtos_2})
    echo -n -e "\x33\x71\xb3\x2b\x17\xca\x43\xe2\xb4\xdb\x11\xe1\x3e\xc0\xa5\xf3" >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    printf "%02x%02x%02x%02x" $(((size) & 0xff)) $((((size) >> 8) & 0xff)) \
        $((((size) >> 16) & 0xff)) $((((size) >> 24) & 0xff)) | xxd -r -ps >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    printf "%02x%02x%02x%02x" $(((offset) & 0xff)) $((((offset) >> 8) & 0xff)) \
        $((((offset) >> 16) & 0xff)) $((((offset) >> 24) & 0xff)) | xxd -r -ps >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    printf "\0\0\0\0\0\0\0\0" >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
    openssl dgst -sha256 -binary ${rtos_2} >>${RTOS_BUILD_DIR}/_tmp_hdr.bin
}

function package_binary() {
    local fw_size=
    local rem_val=
    local bin1=$1
    local bin2=$2
    local out_bin=$3

    # align bin1 size up to 4KB and padding bin2 at the end of bin1
    fw_size=$(stat -c %s ${bin1})
    fw_size=$(expr ${fw_size} + 4095)
    rem_val=$(expr ${fw_size} % 4096)
    fw_size=$(expr ${fw_size} - ${rem_val})
    dd if=/dev/zero of=${RTOS_BUILD_DIR}/_tmp.bin bs=1 count=${fw_size} &>/dev/null
    dd if=${bin1} of=${RTOS_BUILD_DIR}/_tmp.bin bs=1 count=${fw_size} conv=notrunc &>/dev/null

    cat ${bin2} >>${RTOS_BUILD_DIR}/_tmp.bin
    mv ${RTOS_BUILD_DIR}/_tmp.bin ${RTOS_BUILD_DIR}/${out_bin}
    echo ==== package ${bin1} ${bin2} to ${out_bin} ====
}

function package_fastboot() {
if [ -n $UBOOT_DIR ]; then
	pushd $UBOOT_DIR
	if [ -d ./fastboot ]; then
		rm -rf ./fastboot
	fi
	mkdir -p ./fastboot
	cp $RTOS_IMAGE_1 ./fastboot
	cp $RTOS_IMAGE_2 ./fastboot
	cp $BL22_IMAGE ./fastboot
	echo "./mk $UBOOT_CFG --build-nogit --ipc-ddr-size $DDR_SIZE"
	./mk $UBOOT_CFG --build-nogit --ipc-ddr-size $DDR_SIZE
	if [ "$?" -ne 0 ]; then
		if [ -d ./fastboot ]; then
			rm -rf ./fastboot
		fi
		echo "RTOS-SDK: Uboot compilation failed !!!"
		exit 1
	fi

	if [ -d ./fastboot ]; then
		rm -rf ./fastboot
	fi
	popd
fi
    build_header $BL22_IMAGE $RTOS_IMAGE_1 $RTOS_IMAGE_2
    package_binary ${RTOS_BUILD_DIR}/_tmp_hdr.bin ${RTOS_BUILD_DIR}/bl22.bin bl22.bin
    package_binary ${RTOS_BUILD_DIR}/bl22.bin ${RTOS_BUILD_DIR}/rtos_1.bin bl22.bin
    package_binary ${RTOS_BUILD_DIR}/bl22.bin ${RTOS_BUILD_DIR}/rtos_2.bin rtos_fastboot.bin
if [[ -d "$INSTALL_DIR" ]]; then
    cp  ${RTOS_BUILD_DIR}/rtos_fastboot.bin ${INSTALL_DIR} -rf
fi
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
rtos_config_prepare
#choose sensor
sensor_config_choose
#Compile toolchain preparation
toolchain_prepare
#compile the rtos image
cd $RTOS_BASE_DIR && make
if [ "$?" -ne 0 ]; then
    echo "RTOS-SDK: RTOS compilation failed !!!"
    exit 1
fi
#lz4 compression
lz4_rtos
#compile the bl22 image
bl22_compile
#Packaging RTOS binary files
package_fastboot
#debug
# debug_info
