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
	echo "Available boards:"
	find $PWD/boards/ -mindepth 2 -maxdepth 2 -type d ! -name ".*" -exec basename {} \;
	read -p "Choose your board: " BOARD
	# Check BOARD
	if [ -z "$(find $PWD/boards/ -mindepth 2 -maxdepth 2 -type d -name $BOARD)" ]; then
		echo "No such board!"
		return 1
	fi
fi

if [ -n "$2" ]; then
	PRODUCT=$2
else
	unset PRODUCT
	echo ""
fi
if [ -z $PRODUCT ]; then
	echo "Available products:"
	find $PWD/products/ -mindepth 1 -maxdepth 1 -type d ! -name ".*" -exec basename {} \;
	read -p "Choose your product: " PRODUCT
	# Check PRODUCT
	if [ -z "$(find $PWD/products/ -mindepth 1 -maxdepth 1 -type d -name $PRODUCT)" ]; then
		echo "No such product!"
		return 1
	fi
fi

ARCH=`find -name "*$BOARD" -exec dirname {} \; | xargs basename`

case $ARCH in
	arm) COMPILER=gcc;TOOLCHAIN_KEYWORD="arm-none-eabi" ;;
	arm64) COMPILER=gcc;TOOLCHAIN_KEYWORD="aarch64-none-elf" ;;
	riscv) COMPILER=gcc;TOOLCHAIN_KEYWORD="riscv-none" ;;
	*) echo "Failed to identify ARCH $ARCH";return 1;;
esac

export ARCH BOARD COMPILER PRODUCT TOOLCHAIN_KEYWORD
