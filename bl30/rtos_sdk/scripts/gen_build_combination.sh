#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

###############################################################
# Function: generate build combination.
###############################################################

# $1: "arch soc board product"
check_project()
{
	LINE_NR=`sed -n '$=' $BUILD_COMBINATION`
	i=0
	while IFS= read -r LINE; do
		[[ "$1" == "$LINE" ]] && break
		i=$((i+1))
	done < $BUILD_COMBINATION
	[ $i -ge $LINE_NR ] && $i && return 1

	return 0
}

# $1: arch
# $2: soc
# $3: board
# $4: product
check_build_combination()
{
	[ $# -ne 4 ] && return -1

	i=0
	for arch in ${ARCHS[*]}; do
		[[ "$1" == "$arch" ]] && break
		i=$((i+1))
	done
	[ $i -ge ${#ARCHS[*]} ] && return 1

	i=0
	for soc in ${SOCS[*]};do
		[[ "$2" == "$soc" ]] && break
		i=$((i+1))
	done
	[ "$i" -ge ${#SOCS[*]} ] && return 2

	i=0
	for board in ${BOARDS[*]};do
		[[ "$3" == "$board" ]] && break
		i=$((i+1))
	done
	[ "$i" -ge ${#BOARDS[*]} ] && return 3

	i=0
	for product in ${PRODUCTS[*]};do
		[[ "$4" == "$product" ]] && break
		i=$((i+1))
	done
	[ "$i" -ge ${#PRODUCTS[*]} ] && return 4

	return 0
}

unset ARCHS SOCS BOARDS PRODUCTS

ARCHS=($(find $PWD/arch -mindepth 1 -maxdepth 1 -type d ! -name ".*" | xargs basename -a | sort -n))
PRODUCTS=($(find $PWD/products -mindepth 1 -maxdepth 1 -type d ! -name ".*" | xargs basename -a | sort -n))

if [ -e "$PWD/boards/riscv/CMakeLists.txt" ]; then
	SOCS=($(find $PWD/soc -mindepth 2 -maxdepth 2 -type d ! -name ".*" | xargs basename -a | sort -n))
	BOARDS=($(find $PWD/boards -mindepth 2 -maxdepth 2 -type d ! -name ".*" | xargs basename -a | sort -n))
else
	SPLITARCH=($(find $PWD/boards -mindepth 2 -maxdepth 2 -type d ! -name ".*" | grep "$PWD/boards/riscv" | xargs basename -a | sort -n))
	BOARDS=($(find $PWD/boards -mindepth 2 -maxdepth 2 -type d ! -name ".*" | grep -v "$PWD/boards/riscv" | xargs basename -a | sort -n))
	BOARDS+=($(find $PWD/boards -mindepth 3 -maxdepth 3 -type d ! -name ".*" | grep "$PWD/boards/riscv" | xargs basename -a | sort -n))
	SOCS=($(find $PWD/soc -mindepth 2 -maxdepth 2 -type d ! -name ".*" | grep -v "$PWD/soc/riscv" | xargs basename -a | sort -n))
	SOCS+=($(find $PWD/soc -mindepth 3 -maxdepth 3 -type d ! -name ".*" | grep "$PWD/soc/riscv" | xargs basename -a | sort -n))
fi

export BUILD_COMBINATION="$PWD/output/build_combination.txt"

if [ ! -d "$PWD/output" ]; then
	mkdir -p $PWD/output
fi

for arch in ${ARCHS[*]}; do
	BUILD_COMBINATION_INPUT="$PWD/boards/$arch/build_combination.in"
	if [ -e $BUILD_COMBINATION_INPUT ]; then
		if [ $BUILD_COMBINATION -ot $BUILD_COMBINATION_INPUT ]; then
			:> $BUILD_COMBINATION
		fi
	else
		for split in ${SPLITARCH[*]}; do
			BUILD_COMBINATION_INPUT="$PWD/boards/$arch/$split/build_combination.in"
			if [ $BUILD_COMBINATION -ot $BUILD_COMBINATION_INPUT ]; then
				:> $BUILD_COMBINATION
			fi
		done
	fi
done

if [ ! -s "$BUILD_COMBINATION" ]; then
	for arch in ${ARCHS[*]}; do
		BUILD_COMBINATION_INPUT="$PWD/boards/$arch/build_combination.in"
		if [ -e $BUILD_COMBINATION_INPUT ]; then
			while IFS= read -r LINE; do
				a=`echo "$LINE"|awk '{print $1}'`
				s=`echo "$LINE"|awk '{print $2}'`
				b=`echo "$LINE"|awk '{print $3}'`
				p=`echo "$LINE"|awk '{print $4}'`
				check_build_combination $a $s $b $p
				[ "$?" -eq 0 ] && echo $LINE >> $BUILD_COMBINATION
			done < $BUILD_COMBINATION_INPUT
		else
			for split in ${SPLITARCH[*]}; do
				BUILD_COMBINATION_INPUT="$PWD/boards/$arch/$split/build_combination.in"
				while IFS= read -r LINE; do
					a=`echo "$LINE"|awk '{print $1}'`
					s=`echo "$LINE"|awk '{print $2}'`
					b=`echo "$LINE"|awk '{print $3}'`
					p=`echo "$LINE"|awk '{print $4}'`
					check_build_combination $a $s $b $p
					[ "$?" -eq 0 ] && echo $LINE >> $BUILD_COMBINATION
				done < $BUILD_COMBINATION_INPUT
			done
		fi
	done
fi
