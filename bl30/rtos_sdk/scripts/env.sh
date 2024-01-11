#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

###############################################################
# Function: choose board and product, set environment variables.
###############################################################

usage()
{
	echo "Usage: source $BASH_SOURCE [arch_name] [soc_name] [board_name] [product_name]"
	echo "−h: display help."
	echo ""
}

version_lt() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1"; }

CMAKE_FILE=`which cmake`
if [ -x /opt/cmake-3.18.4-Linux-x86_64/bin/cmake ]; then
	export PATH=/opt/cmake-3.18.4-Linux-x86_64/bin/:$PATH
elif [ -n "$CMAKE_FILE" ]; then
	CMAKE_VERSION=`cmake --version`
	pattern="cmake version "
	CMAKE_VERSION=`echo ${CMAKE_VERSION#*${pattern}}`
	pattern=" CMake"
	CMAKE_VERSION=`echo ${CMAKE_VERSION%${pattern}*}`
	if version_lt $CMAKE_VERSION "3.13.1"; then
		echo "cmake version $CMAKE_VERSION < 3.13.1!"
		echo "Please upgrade cmake!"
		return 0
	fi
else
	echo "cmake not found!"
	echo "Please install cmake!"
	return 0
fi

NINJA_FILE=`which ninja`
if [ -z "$NINJA_FILE" ]; then
	echo "ninja not found!"
	echo "Please install ninja!"
	return 0
fi

source scripts/gen_build_combination.sh

if [ -n "$1" ] && [ $1 == "-h" ]; then
	usage
	return 0
elif [ $# -eq 4 ]; then
	PROJECT="$1 $2 $3 $4"
	echo "Choose project:$PROJECT"
elif [ $# -eq 5 ]; then
	PROJECT="$1 $2 $3 $4 $5"
	echo "Choose project:$PROJECT"
else
	unset ARRAY

	while IFS= read -r LINE; do
		ARRAY+=( "$LINE" )
	done < $BUILD_COMBINATION

	echo "Available projects:"
	j=0
	for j in "${!ARRAY[@]}"; do
		NR=$j
		j=$((j+1))
		echo -e "\t$NR. ${ARRAY[$j-1]}"
	done
	read -p "Choose your project: " CHOICE

	# Determine whether it is a digital number
	expr $CHOICE + 1 > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		if [ $CHOICE -le $j ]; then
			PROJECT=${ARRAY[$CHOICE]}
		else
			echo "Wrong choice!"
			return 0
		fi
	else
		PROJECT=$CHOICE
	fi
fi

ARCH=`echo "$PROJECT"|awk '{print $1}'`
SOC=`echo "$PROJECT"|awk '{print $2}'`
BOARD=`echo "$PROJECT"|awk '{print $3}'`
PRODUCT=`echo "$PROJECT"|awk '{print $4}'`
COMPILER=`echo "$PROJECT"|awk '{print $5}'`
# Check current project
check_build_combination $ARCH $SOC $BOARD $PRODUCT
err=$?
[ "$err" -eq 1 ] && echo "Invalid ARCH: $ARCH!" && return $err
[ "$err" -eq 2 ] && echo "Invalid SOC: $SOC!" && return $err
[ "$err" -eq 3 ] && echo "Invalid BOARD: $BOARD!" && return $err
[ "$err" -eq 4 ] && echo "Invalid PRODUCT: $PRODUCT!" && return $err

case $ARCH in
	arm) COMPILER=gcc; TOOLCHAIN_KEYWORD="arm-none-eabi" ;;
	arm64)  if [ "$COMPILER" == "clang+llvm" ]; then
			TOOLCHAIN_KEYWORD="arm"
			TOOLCHAIN_PATH=$PWD/output/toolchains/clang+llvm-arm
		else
			COMPILER="gcc"
			TOOLCHAIN_KEYWORD="aarch64-none-elf"
		fi
	      ;;
	riscv) COMPILER="gcc"; TOOLCHAIN_KEYWORD="riscv-none" ;;
	xtensa) COMPILER="xcc"; TOOLCHAIN_KEYWORD="xt" ;;
	*) echo "Failed to identify ARCH $ARCH";return 1;;
esac

unset SPLIT_ARCH_DIR
for split in ${SPLITARCH[*]}; do
	result=$(find $PWD/boards/$ARCH/$split -type d -name "$BOARD" 2>/dev/null)
	if [ -n "$result" ]; then
		SPLIT_ARCH_DIR=$(basename "$(dirname "$result")")
	fi
done

CONF_FILE="$PWD/products/$PRODUCT/prj.conf"
BACKTRACE_ENABLE=0
if [ $(grep -c "CONFIG_BACKTRACE=y" $CONF_FILE) -ne '0' ]; then
	BACKTRACE_ENABLE=1
fi

KERNEL=freertos

export ARCH BOARD COMPILER KERNEL PRODUCT SOC TOOLCHAIN_KEYWORD TOOLCHAIN_PATH BACKTRACE_ENABLE SPLIT_ARCH_DIR
