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

#Default configuration parameters
SEGMENT_NAME=""
SEGMENT_CONFIG_FILE=""
SEGMENT_RENAME_FILE=""
SEGMENT_RENAME_DIRECTORY=""
SEGMENT_RENAME_IGNORE_LIST=()

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
		ignore_file_flag=false
		for item in "${SEGMENT_RENAME_IGNORE_LIST[@]}"; do
			ignore_file=$(basename "$item")
			if [[ "$file" == *"$ignore_file"* ]]; then
				ignore_file_flag=true
				break
			fi
		done
		if ! $ignore_file_flag; then
			parse_segment_information $file
			rename_target_file_segment $file
		fi
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

arg1=$1

#Script Execution Parameter Parsing
while getopts ":e:c:h:d:n:f:i:" opt; do
case ${opt} in
	c )
	SEGMENT_CONFIG_FILE=$OPTARG
	echo $OPTARG
	;;
	d )
	SEGMENT_RENAME_DIRECTORY=$OPTARG
	;;
	f )
	SEGMENT_RENAME_FILE=$OPTARG
	;;
	n )
	SEGMENT_NAME=$OPTARG
	;;
	i )
	SEGMENT_RENAME_IGNORE_LIST+=($OPTARG)
	;;
	e )
	generate_segment_name $SEGMENT_NAME
	generate_segment_rule_file
	echo "<---generate default rule--->"
	exit 0
	;;
	h )
	show_help
	exit 1
	;;
	\? )
	echo "Invalid option: -$OPTARG" 1>&2
	exit 1
	;;
	: )
	echo "Option -$OPTARG requires an argument" 1>&2
	exit 1
	;;
esac
done
shift $((OPTIND -1))

#Segment Renaming Business Processing
if [[ ${arg1:0:1} != "-" ]]; then
	generate_segment_name $2
	generate_segment_rule_file

	if [ -s "$1" ] && [ -f "$1" ]; then
		parse_segment_information $1
		rename_target_file_segment $1
	elif [ -d "$1" ]; then
		search_and_process_specified_directory $1
	else
		show_help
		exit 1
	fi
else
	generate_segment_name $SEGMENT_NAME
	generate_segment_rule_file

	if [ -s "$SEGMENT_RENAME_FILE" ] && [ -f "$SEGMENT_RENAME_FILE" ]; then
		parse_segment_information $SEGMENT_RENAME_FILE
		rename_target_file_segment $SEGMENT_RENAME_FILE
	elif [ -d "$SEGMENT_RENAME_DIRECTORY" ]; then
		search_and_process_specified_directory $SEGMENT_RENAME_DIRECTORY
	fi
fi
