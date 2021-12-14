#!/bin/bash

###############################################################
# Function: choose board and product, set environment variables.
###############################################################

usage()
{
	echo "Usage: source $BASH_SOURCE [board_name] [product_name]"
	echo "−h: display help."
	echo ""
}

# $1: path
# $2: depth
choose()
{
	echo "Available $1s:"
	ARRAY=($(find $PWD/$1s/ -mindepth $2 -maxdepth $2 -type d ! -name ".*" | xargs basename -a | sort -n))
	for i in "${!ARRAY[@]}";
	do
		echo -e "\t$i. ${ARRAY[$i]}"
	done
	read -p "Choose your $1: " CHOICE
	# Determine whether it is a digital number
	expr $CHOICE + 1 > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		RESULT=${ARRAY[$CHOICE]}
	else
		RESULT=$CHOICE
	fi
	# Check RESULT
	if [ -z "$(find $PWD/$1s/ -mindepth $2 -maxdepth $2 -type d -name $RESULT)" ]; then
		echo "No such $1!"
		return 1
	fi
}

if [ -n "$1" ]; then
	if [ $1 == "-h" ]; then
		usage
		return 0
	else
		BOARD=$1
	fi
else
	unset BOARD
fi
if [ -z $BOARD ]; then
	choose board 2
	[ $? -ne 0 ] && return 1;
	BOARD=$RESULT
fi

if [ -n "$2" ]; then
	PRODUCT=$2
else
	unset PRODUCT
	echo ""
fi
if [ -z $PRODUCT ]; then
	choose product 1
	[ $? -ne 0 ] && return 1;
	PRODUCT=$RESULT
fi

ARCH=`find -not -path "./output/*" -name "*$BOARD" -exec dirname {} \; | xargs basename`

case $ARCH in
	arm) COMPILER=gcc;TOOLCHAIN_KEYWORD="arm-none-eabi" ;;
	arm64) COMPILER=gcc;TOOLCHAIN_KEYWORD="aarch64-none-elf" ;;
	riscv) COMPILER=gcc;TOOLCHAIN_KEYWORD="riscv-none" ;;
	xtensa) COMPILER=xcc;TOOLCHAIN_KEYWORD="xt" ;;
	*) echo "Failed to identify ARCH $ARCH";return 1;;
esac

export ARCH BOARD COMPILER PRODUCT TOOLCHAIN_KEYWORD
