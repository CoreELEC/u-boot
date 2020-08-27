#!/bin/bash

function build_bl2() {
	echo -n "Build bl2...Please wait..."
	local target="$1/bl2.bin"
	local targetv3="$1/bl2.v3.bin"
	# $1: src_folder, $2: bin_folder, $3: soc
	cd $1
	if [ "$3" == "sc2" ]; then
		#echo "Storage with --bl2ex --dpre"
		/bin/bash mk $3 --ddrtype ${CONFIG_DDRFW_TYPE}
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
	local target="$1/bl2e.bin"

	# $1: src_folder, $2: bin_folder, $3: soc
	cd $1
	#echo "Storage without --pxp"
	/bin/bash mk $3
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
	#echo "Storage/Preloading without --pxp"
	/bin/bash mk $3
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
