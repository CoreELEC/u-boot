#!/bin/bash

function build_bl32() {
	echo -n "Build bl32...Please wait... "
	local target="$1/bl32.img"
	local target2="$1/bl32.bin"
	# $1: src_folder, $2: bin_folder, $3: soc
	cd $1
	if [ $CONFIG_CHIPSET_NAME == "s905w2" ]; then
		eval 'CFG_NO_RPMB=y CFG_RSV_MEM_8M=y ./build.sh $3 ${CONFIG_CAS}'
	elif [ $CONFIG_CHIPSET_NAME == "s805x2" ]; then
		eval 'CFG_RSV_MEM_20M=y ./build.sh $3 ${CONFIG_CAS}'
	elif [ $CONFIG_CHIPSET_NAME == "s805c1" || $CONFIG_CHIPSET_NAME == "s805c1eng" ]; then
		eval 'CFG_NO_RPMB=y ./build.sh $3 ${CONFIG_CAS}'
	elif [ $CONFIG_CHIPSET_NAME == "s805x3" ]; then
		echo CONFIG_CHIPSET_NAME is $CONFIG_CHIPSET_NAME
		eval 'CFG_RSV_MEM_16M_WITH_DYN_SHM=y ./build.sh $3 ${CONFIG_CAS}'
	else
		/bin/bash build.sh $3 ${CONFIG_CAS}
	fi
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
