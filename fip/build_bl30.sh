#!/bin/bash

function build_bl30() {
	# $1: src_folder, $2: bin_folder, $3: soc
	# co-work with bl30 build script under bl30.git
	echo -n "Build $3 bl30 of ${BL30_SELECT}...Please wait..."
	local tartget
	if [[ "${BL30_SELECT}" == "t5w_at301" || "${BL30_SELECT}" == "t5w_skt" ]]; then
		echo "Build bl30 from new RTOS SDK."
		local current_dir=$(cd $(dirname $0); pwd)
		local work_dir=${current_dir}/bl30/rtos_sdk
		local output_dir
		local board_arg
		if [ "${BL30_SELECT}" == "t5w_at301" ]; then
			board_arg=at301_t962d4
		else
			board_arg=at309_t962d4
		fi
		source $work_dir/scripts/env.sh $board_arg aocpu && make MANIFEST_FILE="t5w_aocpu_rtos_sdk.xml" -C $work_dir &&    \
			output_dir=$work_dir/output/$BOARD-$PRODUCT/freertos
		target=$output_dir/bl30.bin
		cp -arf $output_dir/freertos.bin $output_dir/bl30.bin
	else
		echo "Build bl30 from src_ao."
		target="$1/bl30.bin"
		cd $1
		#export CROSS_COMPILE=${AARCH32_TOOL_CHAIN}
		local soc=$3
		if [ "$soc" == "gxtvbb" ]; then
			soc="gxtvb"
		fi
		#make clean BOARD=$soc &> /dev/null
		#make BOARD=$soc &> /dev/null
		if [[ "${BL30_SELECT}" != "" ]]; then
			/bin/bash mk ${BL30_SELECT}
		else
			/bin/bash mk $soc
		fi
	fi

	if [ $? != 0 ]; then
		cd ${MAIN_FOLDER}
		echo "Error: Build bl30 failed... abort"
		exit -1
	fi
	echo "Copy target from $target to $2"
	cd ${MAIN_FOLDER}
	cp ${target} $2 -f
	echo "done"
	return
}
