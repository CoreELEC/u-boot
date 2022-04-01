#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

###############################################################
# Function: generate package combination.
###############################################################

# $1: arch
# $2: soc
# $3: board
# $4: product
check_package_combination() {
	i=0
	for arch in ${ARCHS[*]}; do
		[[ "$1" == "$arch" ]] && break
		i=$((i + 1))
	done
	[ $i -ge ${#ARCHS[*]} ] && return 1

	i=0
	for soc in ${SOCS[*]}; do
		[[ "$2" == "$soc" ]] && break
		i=$((i + 1))
	done
	[ "$i" -ge ${#SOCS[*]} ] && return 2

	i=0
	for board in ${BOARDS[*]}; do
		[[ "$3" == "$board" ]] && break
		i=$((i + 1))
	done
	[ "$i" -ge ${#BOARDS[*]} ] && return 3

	i=0
	for product in ${PRODUCTS[*]}; do
		[[ "$4" == "$product" ]] && break
		i=$((i + 1))
	done
	[ "$i" -ge ${#PRODUCTS[*]} ] && return 4

	return 0
}

unset ARCHS SOCS BOARDS PRODUCTS

ARCHS=($(find $PWD/arch -mindepth 1 -maxdepth 1 -type d ! -name ".*" | xargs basename -a | sort -n))
SOCS=($(find $PWD/soc -mindepth 2 -maxdepth 2 -type d ! -name ".*" | xargs basename -a | sort -n))
BOARDS=($(find $PWD/boards -mindepth 2 -maxdepth 2 -type d ! -name ".*" | xargs basename -a | sort -n))
PRODUCTS=($(find $PWD/products -mindepth 1 -maxdepth 1 -type d ! -name ".*" | xargs basename -a | sort -n))

PACKAGE_COMBINATION_INPUT="$PWD/image_packer/package_combination.in"
export PACKAGE_COMBINATION="$PWD/output/package_combination.txt"

if [ ! -d "$PWD/output" ]; then
	mkdir -p $PWD/output
fi
if [ ! -s "$PACKAGE_COMBINATION" ] || [ $PACKAGE_COMBINATION -ot $PACKAGE_COMBINATION_INPUT ]; then
	: >$PACKAGE_COMBINATION
	while IFS= read -r LINE; do
		ARRY=($(echo $LINE | tr ' ' ' '))
		for ((loop = 0; loop < ${#ARRY[@]}; loop += 4)); do
			arch=${ARRY[loop]}
			soc=${ARRY[loop + 1]}
			board=${ARRY[loop + 2]}
			product=${ARRY[loop + 3]}
			check_package_combination $arch $soc $board $product
			[ "$?" -ne 0 ] && echo "package_combination is error!!!" && exit 1
		done
		echo $LINE >>$PACKAGE_COMBINATION
	done <$PACKAGE_COMBINATION_INPUT
fi
