#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#Toolchain
OBJCOPY=${TOOLCHAIN_KEYWORD}-objcopy
OBJDUMP=${TOOLCHAIN_KEYWORD}-objdump
RTOS_BUILD_DIR=$(realpath $(dirname $(readlink -f ${BASH_SOURCE[0]:-$0}))/..)

#Get specified segment name.
function generate_segment_name() {

	SEGMENT_PREFIX="late"

	if [ ! -z "$1" ]; then
		if [[ $1 =~ ^\..* || $1 =~ .*\. ]]; then
			echo -e "\033[41;33m Notice: The segment name should be a pure string, excluding symbols. \033[0m"
			exit 1
		else
			SEGMENT_PREFIX=$1
		fi
	fi
}

#Generate segmented rule file
function generate_segment_rule_file() {

	ARCH_DEFAULT_MAKEFILE=$RTOS_BUILD_DIR/arch/$ARCH/scatter_load.mk
	BOARD_DEFAULT_MAKEFILE=$RTOS_BUILD_DIR/boards/$ARCH/$BOARD/scatter_load.mk

	FIX_PART_STRING="#Object files and dependency definitions\n\
RTOS_SOURCE_IMAGE = \$(kernel_BUILD_DIR)/\$(KERNEL).elf\n\
RTOS_LOAD_A = \$(kernel_BUILD_DIR)/rtos_1.bin\n\
RTOS_LOAD_B = \$(kernel_BUILD_DIR)/rtos_2.bin\n\n\
#toolchain\n\
OBJCOPY:=\$(TOOLCHAIN_KEYWORD)-objcopy\n\n"

	SEGMENT_LOAD_LIST="#Specify link segment\n\
segment_load_list+=.$SEGMENT_PREFIX.data .$SEGMENT_PREFIX.rodata .$SEGMENT_PREFIX.text\n\n"

	GENERATE_TARGET="# Image compilation post-processing\n\
.PHONY: scatter\n\
scatter:\n\
\t@\${OBJCOPY} -O binary \$(addprefix -R ,\$(segment_load_list)) \${RTOS_SOURCE_IMAGE} \${RTOS_LOAD_A};\n\
\t@\${OBJCOPY} -O binary \$(addprefix -j ,\$(segment_load_list)) \${RTOS_SOURCE_IMAGE} \${RTOS_LOAD_B};"

	if [ -f "$BOARD_DEFAULT_MAKEFILE" ]; then
		echo "board default makefile----------------"
		cp $BOARD_DEFAULT_MAKEFILE $RTOS_BUILD_DIR/build_system/scatter_load.mk
	elif [ -f "$ARCH_DEFAULT_MAKEFILE" ]; then
		echo "arch default makefile----------------"
		cp $ARCH_DEFAULT_MAKEFILE $RTOS_BUILD_DIR/build_system/scatter_load.mk
	else
		echo "auto generate makefile----------------"
		echo -e "${FIX_PART_STRING}${SEGMENT_LOAD_LIST}${GENERATE_TARGET}" >$RTOS_BUILD_DIR/build_system/scatter_load.mk
	fi
}

#Parse object file segment information
function parse_segment_information() {
	if [ "$ARCH" = "xtensa" ]; then
		TEXT_SECTION=$(${OBJDUMP} -h $1 | grep -wo '\.text' | sort -u | uniq)
		DATA_SECTION=$(${OBJDUMP} -h $1 | grep -wo '\.data' | sort -u | uniq)
		RODATA_SECTION=$(${OBJDUMP} -h $1 | grep -wo '\.rodata' | sort -u | uniq)
		BSS_SECTION=$(${OBJDUMP} -h $1 | grep -wo '\.bss' | sort -u | uniq)
	else
		TEXT_SECTION=$(${OBJDUMP} -h $1 | grep -wo '\.text[^ ]*' | sort -u | uniq)
		DATA_SECTION=$(${OBJDUMP} -h $1 | grep -wo '\.data[^ ]*' | sort -u | uniq)
		RODATA_SECTION=$(${OBJDUMP} -h $1 | grep -wo '\.rodata[^ ]*' | sort -u | uniq)
		BSS_SECTION=$(${OBJDUMP} -h $1 | grep -wo '\.bss[^ ]*' | sort -u | uniq)
	fi

	TEXT_ARRY=(${TEXT_SECTION// /})
	DATA_ARRY=(${DATA_SECTION// /})
	RODATA_ARRY=(${RODATA_SECTION// /})
	BSS_SECTION=(${BSS_SECTION// /})
}

#Replace the target file segment name
function rename_target_file_segment() {

	RELINK_FLAG="-p"

	for value in ${TEXT_ARRY[@]}; do
		RELINK_FLAG="${RELINK_FLAG}"" --rename-section ${value}=.${SEGMENT_PREFIX}${value}"
	done

	for value in ${DATA_ARRY[@]}; do
		RELINK_FLAG="${RELINK_FLAG}"" --rename-section ${value}=.${SEGMENT_PREFIX}${value}"
	done

	for value in ${RODATA_ARRY[@]}; do
		RELINK_FLAG="${RELINK_FLAG}"" --rename-section ${value}=.${SEGMENT_PREFIX}${value}"
	done

	if [ "$ARCH" = "xtensa" ]; then
		for value in ${BSS_SECTION[@]}; do
			RELINK_FLAG="${RELINK_FLAG}"" --rename-section ${value}=.${SEGMENT_PREFIX}${value}"
		done
	fi

	if [ "$RELINK_FLAG" != "-p" ]; then
		${OBJCOPY} ${RELINK_FLAG} $1 $1
	fi
}

#Search and process target files in the specified directory.
function search_and_process_specified_directory() {

	OBJ_FILES=$(find $1 -type f -name "*.obj")

	for file in $OBJ_FILES; do
		parse_segment_information $file
		rename_target_file_segment $file
	done
}

function show_help() {
	echo -e "\033[41;33m Notice: Invalid parameters, please refer to the following examples. \033[0m"
	echo -e "\e[1;35m [example]\e[0m"
	echo "    (1) Rename static library section names to default name(.late)."
	echo "    $0 example.a"
	echo "    (2) Rename static library section names to specified names(.sram)."
	echo "    $0 example.a sram"
	echo "    (3) Rename program section names of target files in the specified directory."
	echo "    $0 /example/obj"
	echo "    (4) Rename the section names of target files in the specified directory to the specified names."
	echo "    $0 /example/obj sram"
	exit 1
}

generate_segment_name $2
generate_segment_rule_file

if [ -s "$1" ] && [ -f "$1" ]; then
	parse_segment_information $1
	rename_target_file_segment $1
elif [ -d "$1" ]; then
	search_and_process_specified_directory $1
elif [ "$1" = "-empty" ]; then
	echo "<---generate empty rule--->"
else
	show_help
	exit 1
fi
