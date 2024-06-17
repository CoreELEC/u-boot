#!/bin/bash

# static
declare BLX_BIN_SUB_CHIP="${CONFIG_CHIPSET_NAME}"

if [ -n "${CONFIG_CHIPSET_VARIANT_MIN}" ]; then
	## trunk bl32 bin in 429 409_emmc
	declare CHIPSET_VARIANT_MIN_SUFFIX=".${CONFIG_CHIPSET_VARIANT_MIN}"
fi

if [ -n "${SCRIPT_ARG_CHIPSET_VARIANT}" ]; then
	declare CHIPSET_VARIANT_SUFFIX=".${SCRIPT_ARG_CHIPSET_VARIANT}"
## not trunk, use fastboot/ipc branch
elif [ -n "${CONFIG_CHIPSET_VARIANT}" ]; then
	## bl2e bl32 bl40 bin name with fastboot or null
	declare CHIPSET_VARIANT_SUFFIX=".${CONFIG_CHIPSET_VARIANT}"
	## choose bb1st bin  name is fastboot or ipc
	if [ "${CONFIG_IPC_TYPE}" == "normal" ]; then
		declare CHIPSET_VARIANT_SUFFIX_IPC=".ipc"
	elif [ "${CONFIG_IPC_TYPE}" == "fastboot" ]; then
		declare CHIPSET_VARIANT_SUFFIX_IPC=".fastboot"
	else
		echo "Not setting the correct range CONFIG_IPC_TYPE ${CONFIG_IPC_TYPE}"
		declare CHIPSET_VARIANT_SUFFIX_IPC=".${CONFIG_CHIPSET_VARIANT}"
	fi
	if [ "${CONFIG_IPC_DDR_SIZE}" == "256m" ]; then
		## choose bl2x bl31 bin name witch fastboot_256, otherwise it is fastboot)
		declare CHIPSET_VARIANT_SUFFIX_DDR=".${CONFIG_CHIPSET_VARIANT}_256"
		if [ "${CONFIG_TEE_TYPE}" == "8m" ]; then
			## fastboot/ipc use bl32 bin in 402 409
			CHIPSET_VARIANT_MIN_SUFFIX=".8m"
		elif [ "${CONFIG_TEE_TYPE}" == "1m" ]; then
			## Will not enter here
			CHIPSET_VARIANT_MIN_SUFFIX=".1m"
		fi
	else
		declare CHIPSET_VARIANT_SUFFIX_DDR=".${CONFIG_CHIPSET_VARIANT}"
	fi
else
	declare CHIPSET_VARIANT_SUFFIX=""
fi
declare -a BLX_NAME=("bl2"	\
		     "bl2"	\
		     "bl2e"	\
		     "bl2e"	\
		     "bl2x"	\
		     "bl31"	\
		     "bl32"	\
		     "bl40")

declare -a BLX_SRC_FOLDER=("bl2/core"		\
			   "bl2/core"		\
			   "bl2/ree"		\
			   "bl2/ree"		\
			   "bl2/tee"		\
			   "bl31/bl31_1.3/src"	\
			   "bl32/bl32_3.8/src"	\
			   "NULL"		\
			   "bl33")

declare -a BLX_BIN_FOLDER=("bl2/bin"		\
			   "bl2/bin"		\
			   "bl2/bin"		\
			   "bl2/bin"		\
			   "bl2/bin"		\
			   "bl31/bl31_1.3/bin"	\
			   "bl32/bl32_3.8/bin"	\
			   "bl40/bin")

if [ "y" == "${CONFIG_PXP_NO_SIGNED}" ]; then
declare -a BLX_BIN_NAME=("bl2.bin.sto"	\
			    "bl2.bin.usb"	\
			    "bl2e.bin.sto"	\
			    "bl2e.bin.usb"	\
			    "bl2x.bin"		\
			    "bl31.bin"		\
			    "bl32.bin"		\
			    "bl40.bin")

else
declare -a BLX_BIN_NAME=("bb1st.sto${CHIPSET_VARIANT_SUFFIX_IPC}.bin.signed"     \
			 "bb1st.usb${CHIPSET_VARIANT_SUFFIX_IPC}.bin.signed"     \
			 "blob-bl2e.sto${CHIPSET_VARIANT_SUFFIX}.bin.signed" \
			 "blob-bl2e.usb${CHIPSET_VARIANT_SUFFIX}.bin.signed" \
			 "blob-bl2x${CHIPSET_VARIANT_SUFFIX_DDR}.bin.signed"     \
			 "blob-bl31${CHIPSET_VARIANT_SUFFIX_DDR}.bin.signed"     \
			 "blob-bl32${CHIPSET_VARIANT_MIN_SUFFIX}${CHIPSET_VARIANT_SUFFIX}.bin.signed" \
			 "blob-bl40${CHIPSET_VARIANT_SUFFIX}.bin.signed")
fi

## c3 old aw402s
if [ "" != "${CHIPSET_VARIANT_MIN_SUFFIX}" ] && [ "${CONFIG_TEE_TYPE}" == "" ] && [ "fastboot" == "${CONFIG_CHIPSET_VARIANT}" ]; then
	declare -a BLX_BIN_SIZE=("169984"	\
				 "169984"	\
				 "74864"	\
				 "74864"	\
				 "66672"	\
				 "98304"	\
				 "528384"	\
				 "8192")
	declare BL30_BIN_SIZE="65536"
	declare BL33_BIN_SIZE="524288"
## fastboot/ipc branch (402 409)
elif [ "fastboot" == "${CONFIG_CHIPSET_VARIANT}" ]; then
	## c3 support bl32 in 256m
	if [ "${CONFIG_IPC_DDR_SIZE}" == "256m" ] && [ "${CONFIG_TEE_TYPE}" == "8m" ]; then
		declare -a BLX_BIN_SIZE=("169984"	\
				"169984"	\
				"74864"	\
				"74864"	\
				"66672"	\
				"98304"	\
				"528384"	\
				"8192")
	## c3 not support bl32
	else
		declare -a BLX_BIN_SIZE=("169984"	\
				"169984"	\
				"74864"	\
				"74864"	\
				"66672"	\
				"98304"	\
				"8192"	\
				"8192")

	fi
	declare BL30_BIN_SIZE="4096"
	export CONFIG_BL33_SIZE
	if [ "${CONFIG_BL33_SIZE}" = "s" ]; then
		## +64k
		declare BL33_BIN_SIZE="389120"
	elif [ "${CONFIG_BL33_SIZE}" = "m" ]; then
		## +128k
		declare BL33_BIN_SIZE="454656"
	else
		declare BL33_BIN_SIZE="323584"
	fi
else
## c3 trunk 419 429 409_emmc
	declare -a BLX_BIN_SIZE=("169984"	\
				"169984"	\
				"74864"	\
				"74864"	\
				"66672"	\
				"266240"	\
				"528384"	\
				"102400")
	declare BL30_BIN_SIZE="65536"
	declare BL33_BIN_SIZE="1572864"
fi

declare DEV_ACS_BIN_SIZE="4096"
declare -a BLX_RAWBIN_NAME=("bl2.bin.sto"	\
				"bl2.bin.usb"	\
				"bl2e.bin.sto"	\
				"bl2e.bin.usb"	\
				"bl2x.bin"		\
				"bl31.bin"		\
				"bl32.bin"		\
				"bl40.bin")

declare -a BLX_IMG_NAME=("NULL"	\
			 "NULL"	\
			 "NULL"	\
			 "NULL"	\
			 "NULL"	\
			 "NULL"	\
			 "NULL")

declare -a BLX_NEEDFUL=("true"	\
			"true"	\
			"true"	\
			"true"	\
			"true"	\
			"ture"	\
			"true")

declare -a BLX_SRC_GIT=("bootloader/amlogic-advanced-bootloader/core" \
			"bootloader/amlogic-advanced-bootloader/core" \
			"bootloader/amlogic-advanced-bootloader/ree" \
			"bootloader/amlogic-advanced-bootloader/ree" \
			"bootloader/amlogic-advanced-bootloader/tee" \
			"ARM-software/arm-trusted-firmware" \
			"OP-TEE/optee_os" \
			"uboot")

declare -a BLX_BIN_GIT=("firmware/bin/bl2" \
			"firmware/bin/bl2" \
			"firmware/bin/bl2" \
			"firmware/bin/bl2" \
			"firmware/bin/bl2" \
			"firmware/bin/bl31" \
			"firmware/bin/bl32" \
			"firmware/bin/b40")

# blx priority. null: default, source: src code, others: bin path
declare -a BIN_PATH=("null"	\
		     "null"	\
		     "null"	\
		     "null"	\
		     "null"	\
		     "null"	\
		     "null"	\
		     "null")

# variables
declare -a CUR_REV # current version of each blx
declare -a BLX_READY=("false",	\
		      "false",	\
		      "false",	\
		      "false",	\
		      "false",	\
		      "false",	\
		      "false",	\
		      "false") # blx build/get flag

# package variables
declare BL33_COMPRESS_FLAG=""
declare BL3X_SUFFIX="bin"
declare V3_PROCESS_FLAG=""
declare FIP_ARGS=""
declare AML_BL2_NAME=""
declare AML_KEY_BLOB_NAME=""
declare FIP_BL32_PROCESS=""
declare BOOT_SIG_FLAG=""
declare EFUSE_GEN_FLAG=""
declare DDRFW_TYPE=""

BUILD_PATH=${FIP_BUILD_FOLDER}
BUILD_PAYLOAD=${FIP_BUILD_FOLDER}/payload
CHIPSET_TEMPLATES_PATH="soc/templates"
CONFIG_DDR_FW=0
DDR_FW_NAME="aml_ddr.fw"

CONFIG_NEED_BL32=y
ADVANCED_BOOTLOADER=1

BL2X_BL31_BRANCH="projects/fastboot/c3"
BL2X_BL31_256_BRANCH="projects/fastboot/c3_256"
local find_base='0'

function get_branch() {
	local oldifs="$IFS"
	local base_branch=""
	local tmp=""
	IFS=$'\n'

	tmp=`git branch`
	# Eg: * (HEAD detached at firmware/projects/sc2)
	if [[ "${tmp}" =~ "HEAD detached " ]]; then
		tmp=`git branch -vv | grep '^\*.*\[.*\]' | awk '{print $5}'`
	else
		tmp=`git branch -vv | grep '^\*.*\[.*\]' | awk '{print $4}'`
	fi

	if [ "${CONFIG_IPC_DDR_SIZE}" == "256m" ] && [[ "${tmp}" =~ "${BL2X_BL31_256_BRANCH}" ]]; then
		base_branch=${BL2X_BL31_256_BRANCH}
		find_base=1
	elif [ "${CONFIG_IPC_DDR_SIZE}" == "128m" ] && [[ "${tmp}" =~ "${BL2X_BL31_BRANCH}" ]] && [[ ! "${tmp}" =~ "${BL2X_BL31_256_BRANCH}" ]]; then
		base_branch=${BL2X_BL31_BRANCH}
		find_base=1
	fi
	IFS="$oldifs"
	CURRENT_BL_BRANCH=${base_branch}
	export CURRENT_BL_BRANCH
	echo "CURRENT_BL_BRANCH ${CURRENT_BL_BRANCH}"
	return
}

# Check the correct use of (aw402/aw409) bl2x bl31 branch for ddr size
function check_branch_bl2x_bl31() {
	if [ -n "${CONFIG_CHIPSET_VARIANT}" ] && [ -z "${CONFIG_CHIPSET_VARIANT_MIN}" ]; then
		if [ "${CONFIG_IPC_DDR_SIZE}" == "256m" ]; then
			local dest_branch=${BL2X_BL31_256_BRANCH}
		elif [ "${CONFIG_IPC_DDR_SIZE}" == "128m" ]; then
			local dest_branch=${BL2X_BL31_BRANCH}
		else
			echo -e "Not setting the correct range CONFIG_IPC_DDR_SIZE ${CONFIG_IPC_DDR_SIZE}"
			exit -1
		fi
		local str=`git branch --remote | grep ${dest_branch}`

		# 1, check if existed amlogic git branch name format
		if [ "${str}" == "" ]; then
			echo "can't find ${dest_branch}"
		else
			local cur_branch=''
			local diff=`git diff`

			# 2, check current branch is based on target soc?
			get_branch
			if [ "${find_base}" -eq 0 ]; then
				echo ==== BRANCH ${dest_branch} not found ====
				exit -1
			fi
			find_base=0  # reset find_base
			echo ==== current branch:${CURRENT_BL_BRANCH} ====
			if [[ "${CURRENT_BL_BRANCH}" == *"${dest_branch}" ]]; then
				echo ==== NO NEED TO SWITCH BRANCH ====
				return
			else
				echo ==== NEED TO SWITCH BRANCH ${dest_branch}====
				exit -1
			fi

		fi
	fi
}
