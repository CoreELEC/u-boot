#!/bin/bash

###############################################################
# Function: choose board and product, set environment variables.
###############################################################

BUILD_COMBINATION="$PWD/scripts/build_combination.txt"

usage()
{
	echo "Usage: source $BASH_SOURCE [board_name] [soc_name] [arch_name] [product_name]"
	echo "−h: display help."
	echo ""
}

if [ -n "$1" ]; then
	if [ $1 == "-h" ]; then
		usage
		return 0
	else
		PROJECT="$1 $2 $3 $4"
	fi
else
	unset ARRAY

	while IFS= read -r LINE; do
		ARRAY+=( "$LINE" )
	done < "$BUILD_COMBINATION"

	echo "Available projects:"
	i=0
	for i in "${!ARRAY[@]}";
	do
		echo -e "\t$i. ${ARRAY[$i]}"
	done
	read -p "Choose your project: " CHOICE

	# Determine whether it is a digital number
	expr $CHOICE + 1 > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		if [ $CHOICE -le $i ]; then
			PROJECT=${ARRAY[$CHOICE]}
		else
			echo "Wrong choice!"
			return 0
		fi
	else
		PROJECT=$CHOICE
	fi
fi

ARCH=`echo "$PROJECT"|awk '{print $3}'`
if [ -z "$(find $PWD/arch -mindepth 1 -maxdepth 1 -type d -name $ARCH)" ]; then
	echo "Invalid ARCH: $ARCH!"
	return 1
fi

SOC=`echo "$PROJECT"|awk '{print $2}'`
if [ -z "$(find $PWD/soc/$ARCH -mindepth 1 -maxdepth 1 -type d -name $SOC)" ]; then
	echo "No such SoC: $SOC!"
	return 1
fi

BOARD=`echo "$PROJECT"|awk '{print $1}'`
if [ -z "$(find $PWD/boards/$ARCH -mindepth 1 -maxdepth 1 -type d -name $BOARD)" ]; then
	echo "No such board: $BOARD!"
	return 1
fi

PRODUCT=`echo "$PROJECT"|awk '{print $4}'`
if [ -z "$(find $PWD/products -mindepth 1 -maxdepth 1 -type d -name $PRODUCT)" ]; then
	echo "No such product: $PRODUCT!"
	return 1
fi

case $ARCH in
	arm) COMPILER=gcc;TOOLCHAIN_KEYWORD="arm-none-eabi" ;;
	arm64) COMPILER=gcc;TOOLCHAIN_KEYWORD="aarch64-none-elf" ;;
	riscv) COMPILER=gcc;TOOLCHAIN_KEYWORD="riscv-none" ;;
	xtensa) COMPILER=xcc;TOOLCHAIN_KEYWORD="xt" ;;
	*) echo "Failed to identify ARCH $ARCH";return 1;;
esac

export ARCH BOARD COMPILER PRODUCT SOC TOOLCHAIN_KEYWORD
