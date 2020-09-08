#!/bin/bash

advanced_bootloader_soc="sc2 t7 s4"
# check if soc is using advanced boot loader
function is_abs() {
	local t=$(echo $advanced_bootloader_soc | grep -w "$1")
	if [ "$t" == "" ]; then
		ADVANCED_BOOTLOADER=0
	else
		ADVANCED_BOOTLOADER=1
	fi
	export ADVANCED_BOOTLOADER
}


function build_bl2() {
	echo -n "Build bl2...Please wait..."
	local target="$1/bl2.bin"
	local targetv3="$1/bl2.v3.bin"
	# $1: src_folder, $2: bin_folder, $3: soc
	is_abs $3
	cd $1
	if [ "$ADVANCED_BOOTLOADER" == "1" ]; then
		#echo "Storage with --bl2ex --dpre"
		/bin/bash mk $3 --ddrtype ${CONFIG_DDRFW_TYPE} --dsto
		/bin/bash mk $3 --ddrtype ${CONFIG_DDRFW_TYPE} --dusb
		/bin/bash mk $3 --ddrtype ${CONFIG_DDRFW_TYPE}
		target="$1/bl2.bin*"
		targetv3="$1/chip_acs.bin"
	elif [ "$3" == "t7" ]; then
		#echo "Storage with --pxp --dpre"
		/bin/bash mk $3 --pxp
		targetv3="$1/chip_acs.bin"
	else
		/bin/bash mk $3
	fi
	if [ $? != 0 ]; then
		cd ${MAIN_FOLDER}
		echo "Error: Build bl2 failed... abort"
		exit -1
	fi
	cd ${MAIN_FOLDER}
	cp ${target} $2 -f
	if [ -e ${targetv3} ]; then
		cp ${targetv3} $2 -f
	fi
	echo "...done"
	return
}

function build_bl2e() {
	echo -n "Build bl2e...Please wait..."
	local target="$1/bl2e.bin*"

	# $1: src_folder, $2: bin_folder, $3: soc
	cd $1
	if [ "$3" == "t7" ]; then
		#echo "Storage with --pxp --dpre"
		/bin/bash mk $3 --pxp
	else
		#echo "Storage without --pxp"
		/bin/bash mk $3
	fi

	if [ $? != 0 ]; then
		cd ${MAIN_FOLDER}
		echo "Error: Build bl2 failed... abort"
		exit -1
	fi
	cd ${MAIN_FOLDER}
	cp ${target} $2 -f
	echo "...done"
	return
}

function build_bl2x() {
	echo -n "Build bl2...Please wait..."
	local target="$1/bl2x.bin"

	# $1: src_folder, $2: bin_folder, $3: soc
	cd $1
	if [ "$3" == "t7" ]; then
		#echo "Storage with --pxp --dpre"
		/bin/bash mk $3 --pxp
	else
		#echo "Storage/Preloading without --pxp"
		/bin/bash mk $3
	fi

	if [ $? != 0 ]; then
		cd ${MAIN_FOLDER}
		echo "Error: Build bl2x failed... abort"
		exit -1
	fi
	cd ${MAIN_FOLDER}
	cp ${target} $2 -f
	echo "...done"
	return
}
