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

# $1: arch
# $2: soc
# $3: board
# $4: product
check_params()
{
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
SOCS=($(find $PWD/soc -mindepth 2 -maxdepth 2 -type d ! -name ".*" | xargs basename -a | sort -n))
BOARDS=($(find $PWD/boards -mindepth 2 -maxdepth 2 -type d ! -name ".*" | xargs basename -a | sort -n))
PRODUCTS=($(find $PWD/products -mindepth 1 -maxdepth 1 -type d ! -name ".*" | xargs basename -a | sort -n))

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
	j=0
	for j in "${!ARRAY[@]}"; do
		NR=$j
		ARCH=`echo "${ARRAY[$j]}"|awk '{print $3}'`
		SOC=`echo "${ARRAY[$j]}"|awk '{print $2}'`
		BOARD=`echo "${ARRAY[$j]}"|awk '{print $1}'`
		PRODUCT=`echo "${ARRAY[$j]}"|awk '{print $4}'`
		j=$((j+1))
		check_params $ARCH $SOC $BOARD $PRODUCT
		[ "$?" -ne 0 ] && continue

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

ARCH=`echo "$PROJECT"|awk '{print $3}'`
SOC=`echo "$PROJECT"|awk '{print $2}'`
BOARD=`echo "$PROJECT"|awk '{print $1}'`
PRODUCT=`echo "$PROJECT"|awk '{print $4}'`
check_params $ARCH $SOC $BOARD $PRODUCT
err=$?
[ "$err" -eq 1 ] && echo "Invalid ARCH: $ARCH!" && return $err
[ "$err" -eq 2 ] && echo "Invalid SOC: $SOC!" && return $err
[ "$err" -eq 3 ] && echo "Invalid BOARD: $BOARD!" && return $err
[ "$err" -eq 4 ] && echo "Invalid PRODUCT: $PRODUCT!" && return $err

case $ARCH in
	arm) COMPILER=gcc;TOOLCHAIN_KEYWORD="arm-none-eabi" ;;
	arm64) COMPILER=gcc;TOOLCHAIN_KEYWORD="aarch64-none-elf" ;;
	riscv) COMPILER=gcc;TOOLCHAIN_KEYWORD="riscv-none" ;;
	xtensa) COMPILER=xcc;TOOLCHAIN_KEYWORD="xt" ;;
	*) echo "Failed to identify ARCH $ARCH";return 1;;
esac

export ARCH BOARD COMPILER PRODUCT SOC TOOLCHAIN_KEYWORD
