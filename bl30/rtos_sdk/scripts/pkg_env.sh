#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

###############################################################
# Function: choose board and product, set environment variables.
###############################################################

usage() {
	echo "Usage: source $BASH_SOURCE [arch_name] [soc_name] [board_name] [product_name]"
	echo "−h: display help."
	echo ""
}

version_lt() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1"; }

CMAKE_FILE=$(which cmake)
if [ -x /opt/cmake-3.18.4-Linux-x86_64/bin/cmake ]; then
	export PATH=/opt/cmake-3.18.4-Linux-x86_64/bin/:$PATH
elif [ -n "$CMAKE_FILE" ]; then
	CMAKE_VERSION=$(cmake --version)
	pattern="cmake version "
	CMAKE_VERSION=$(echo ${CMAKE_VERSION#*${pattern}})
	pattern=" CMake"
	CMAKE_VERSION=$(echo ${CMAKE_VERSION%${pattern}*})
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

NINJA_FILE=$(which ninja)
if [ -z "$NINJA_FILE" ]; then
	echo "ninja not found!"
	echo "Please install ninja!"
	return 0
fi

source scripts/gen_package_combination.sh

if [ -n "$1" ] && [ $1 == "-h" ]; then
	usage
	return 0
elif [ $# -eq 2 ] && [ $2 == "gen_all" ]; then
	CHOICE_PACKAGE=$1
	echo "Choose project:$CHOICE_PACKAGE"
else
	unset CHOICE_PACKAGE

	if [ -z "$CHOICE_PACKAGE" ]; then
		echo "Available projects:"
		#show all package combinations
		j=0
		while IFS= read -r LINE; do
			NR=$j
			echo -e "\t$NR. ${LINE[@]}"
			j=$((j + 1))
		done <$PACKAGE_COMBINATION
		read -p "Choose your project: " CHOICE_PACKAGE
		# Determine whether it is a digital number
		expr $CHOICE_PACKAGE + 1 >/dev/null 2>&1
		if [[ $? -ne 0 ]] || [[ $CHOICE_PACKAGE -ge $j ]]; then
			echo "Wrong choice!"
			return 0
		fi
	fi
fi

KERNEL=freertos

export KERNEL CHOICE_PACKAGE
