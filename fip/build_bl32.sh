#!/bin/bash

function build_bl32() {
	echo -n "Build bl32...Please wait... "
	local target="$1/bl32.img"
	local target2="$1/bl32.bin"
	# $1: src_folder, $2: bin_folder, $3: soc
	cd $1
	/bin/bash build.sh $3 ${CONFIG_CAS}
	if [ $? != 0 ]; then
		cd ${MAIN_FOLDER}
		echo "Error: Build bl32 failed... abort"
		exit -1
	fi
	cd ${MAIN_FOLDER}
	cp ${target} $2 -f
	if [ "$ADVANCED_BOOTLOADER" == "1" ]; then
		cp ${target2} $2 -f
	fi
	echo "done"
	return
}
