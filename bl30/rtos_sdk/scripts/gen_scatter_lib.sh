#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#Toolchain
OBJCOPY=${TOOLCHAIN_KEYWORD}-objcopy

#Parse object file segment information
function parse_segment_information() {
	TEXT_SECTION=$(readelf -t $1 | grep -wo .text.* | sort -u | uniq)
	DATA_SECTION=$(readelf -t $1 | grep -wo .data.* | sort -u | uniq)
	RODATA_SECTION=$(readelf -t $1 | grep -wo .rodata.* | sort -u | uniq)
	TEXT_ARRY=(${TEXT_SECTION// /})
	DATA_ARRY=(${DATA_SECTION// /})
	RODATA_ARRY=(${RODATA_SECTION// /})
}

#Replace the target file segment name
function rename_target_file_segment() {
	RELINK_FLAG="-p"
	for value in ${TEXT_ARRY[@]}; do
		RELINK_FLAG="${RELINK_FLAG}"" --rename-section ${value}=.late${value}"
	done

	for value in ${DATA_ARRY[@]}; do
		RELINK_FLAG="${RELINK_FLAG}"" --rename-section ${value}=.late${value}"
	done

	for value in ${RODATA_ARRY[@]}; do
		RELINK_FLAG="${RELINK_FLAG}"" --rename-section ${value}=.late${value}"
	done

	${OBJCOPY} ${RELINK_FLAG} $1 $1
}

if [ -z "$1" ]; then
	echo -e "\033[41;33m Notice: .a library file not specified \033[0m"
	echo -e "\033[33m usage: ./gen_scatter_lib.sh xxx.a \033[0m"
	exit 1
elif [ ! -f "$1" ]; then
	echo -e "\033[41;33m Notice: Please specify a valid library file \033[0m"
	exit 1
else
	parse_segment_information $1
	rename_target_file_segment $1
fi
