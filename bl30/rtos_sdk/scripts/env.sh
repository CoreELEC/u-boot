#!/bin/bash

###############################################################
# Function: choose board and product.
###############################################################

echo "Available boards:"
find $PWD/boards/ -mindepth 2 -maxdepth 2 -type d ! -name ".*" -exec basename {} \;
read -p "Choose your board: " BOARD

echo ""

echo "Available products:"
find $PWD/products/ -mindepth 1 -maxdepth 1 -type d ! -name ".*" -exec basename {} \;
read -p "Choose your product: " PRODUCT

ARCH=`find -name "*$BOARD" -exec dirname {} \; | xargs basename`

case $ARCH in
	arm) COMPILER=gcc;TOOLCHAIN_KEYWORD="arm-none-eabi" ;;
	arm64) COMPILER=gcc;TOOLCHAIN_KEYWORD="aarch64-none-elf" ;;
	riscv) COMPILER=gcc;TOOLCHAIN_KEYWORD="riscv-none" ;;
	*) echo "Failed to identify ARCH $ARCH" ;;
esac

export ARCH BOARD COMPILER PRODUCT TOOLCHAIN_KEYWORD
