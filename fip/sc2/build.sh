#!/bin/bash

# include uboot pre-build macros
#declare CONFIG_FILE=("${buildtree}/.config")
#declare AUTOCFG_FILE=("${buildtree}/include/autoconf.mk")

function init_vari() {
	#source ${CONFIG_FILE} &> /dev/null # ignore warning/error
	#source ${AUTOCFG_FILE} &> /dev/null # ignore warning/error

	AML_BL2_NAME="bl2.bin"
	AML_KEY_BLOB_NANE="aml-user-key.sig"

	if [ "y" == "${CONFIG_AML_SECURE_BOOT_V3}" ]; then
		V3_PROCESS_FLAG="--level v3"
	fi

	if [ "y" == "${CONFIG_AML_CRYPTO_AES}" ]; then
		BOOT_SIG_FLAG="--aeskey enable"
		EFUSE_GEN_FLAG="--aeskey enable"
	fi

	if [ "y" == "${CONFIG_AML_EFUSE_GEN_AES_ONLY}" ]; then
		EFUSE_GEN_FLAG="--aeskey only"
	fi

	if [ "y" == "${CONFIG_AML_BL33_COMPRESS_ENABLE}" ]; then
		BL33_COMPRESS_FLAG="--compress lz4"
	fi

	if [ "y" == "${CONFIG_FIP_IMG_SUPPORT}" ]; then
		BL3X_SUFFIX="img"
	fi
}

function mk_bl2ex() {
	output=$1
	payload=$2
	ddr_type=$3

	if [ ! -f ${output}/bl2.bin ]	|| \
	   [ ! -f ${output}/bl2e.bin ]	|| \
	   [ ! -f ${output}/bl2x.bin ]; then
		echo "Error: ${output}/bl2/e/x.bin does not all exist... abort"
		ls -la ${output}
		exit -1
	fi

	echo "================================================================="
	echo "image packing with acpu-imagetool for bl2 bl2e bl2x"

	dd if=/dev/zero of=${payload}/bl2.bin bs=127904 count=1
	dd if=${output}/bl2.bin of=${payload}/bl2.bin conv=notrunc

	dd if=/dev/zero of=${payload}/bl2e.bin bs=65536 count=1
	dd if=${output}/bl2e.bin of=${payload}/bl2e.bin conv=notrunc

	dd if=/dev/zero of=${payload}/bl2x.bin bs=65536 count=1
	dd if=${output}/bl2x.bin of=${payload}/bl2x.bin conv=notrunc


	echo "===================================================="
	echo "------ process for Synopsys ddr fw ------"
	INPUT_DDRFW=./${FIP_FOLDER}${CUR_SOC}

	if [ "$ddr_type" == "ddr4" ]; then
		dd if=${INPUT_DDRFW}/ddr4_1d.fw of=${payload}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=${INPUT_DDRFW}/ddr4_2d.fw of=${payload}/ddrfw_2d.bin skip=96 bs=1 count=36864
	elif [ "$ddr_type" == "ddr3" ]; then
		dd if=${INPUT_DDRFW}/ddr3_1d.fw of=${payload}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=/dev/zero of=${payload}/ddrfw_2d.bin bs=36864 count=1
	elif [ "$ddr_type" == "lpddr4" ]; then
		dd if=${INPUT_DDRFW}/lpddr4_1d.fw of=${payload}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=${INPUT_DDRFW}/lpddr4_2d.fw of=${payload}/ddrfw_2d.bin skip=96 bs=1 count=36864
	elif [ "$ddr_type" == "lpddr3" ]; then
		dd if=${INPUT_DDRFW}/lpddr3_1d.fw of=${payload}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=/dev/zero of=${payload}/ddrfw_2d.bin bs=36864 count=1
	else
		echo "un-recognized ddr_type: ${ddr_type}"
		echo "---- use default ddr4 ----"
		dd if=${INPUT_DDRFW}/ddr4_1d.fw of=${payload}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=${INPUT_DDRFW}/ddr4_2d.fw of=${payload}/ddrfw_2d.bin skip=96 bs=1 count=36864
	fi

	piei_size=`stat -c %s ${INPUT_DDRFW}/piei.fw`
	if [ $piei_size -gt 12384 ]; then
		dd if=${INPUT_DDRFW}/piei.fw of=${payload}/ddrfw_piei.bin skip=96 bs=1 count=12288
	else
		dd if=/dev/zero of=${payload}/ddrfw_piei.bin bs=12288 count=1
		dd if=${INPUT_DDRFW}/piei.fw of=${payload}/ddrfw_piei.bin skip=96 bs=1 conv=notrunc
	fi

	cat ${payload}/ddrfw_1d.bin ${payload}/ddrfw_2d.bin \
		${payload}/ddrfw_piei.bin > ${payload}/ddrfw_data.bin

	if [ ! -f ${payload}/ddrfw_data.bin ]; then
		echo "ddrfw_data payload does not exist in ${payload} !"
		exit -1
	fi
	ddrfw_data_size=`stat -c %s ${payload}/ddrfw_data.bin`
	if [ $ddrfw_data_size -ne 86016 ]; then
		echo "ddr_fwdata size is not equal to 84K, $ddrfw_data_size"
		exit -1
	fi


	echo "===================================================="
	echo "------ process for device and chip params ------"
	INPUT_PARAMS=${output}

	if [ ! -f ${INPUT_PARAMS}/device_acs.bin ]; then
		echo "dev acs params not exist !"
		exit -1
	fi

	if [ ! -f ${INPUT_PARAMS}/chip_acs.bin ]; then
		echo "chip acs params not exist !"
		exit -1
	fi
	chip_acs_size=`stat -c %s ${INPUT_PARAMS}/chip_acs.bin`
	dev_acs_size=`stat -c %s ${INPUT_PARAMS}/device_acs.bin`

	if [ $chip_acs_size -gt 4096 ]; then
		echo "chip acs size exceed limit 4096, $chip_acs_size"
		exit -1
	else
		dd if=/dev/zero of=${payload}/chip_acs.bin bs=4096 count=1
		dd if=${INPUT_PARAMS}/chip_acs.bin of=${payload}/chip_acs.bin conv=notrunc
	fi

	if [ $dev_acs_size -gt 28672 ]; then
		echo "chip acs size exceed limit 28672, $dev_acs_size"
		exit -1
	else
		dd if=/dev/zero of=${payload}/device_acs.bin bs=28672 count=1
		dd if=${INPUT_PARAMS}/device_acs.bin of=${payload}/device_acs.bin conv=notrunc
	fi

	./${FIP_FOLDER}${CUR_SOC}/acpu-imagetool create-boot-blobs \
			--infile-bl2-payload=${payload}/bl2.bin \
			--infile-bl2e-payload=${payload}/bl2e.bin \
			--infile-bl2x-payload=${payload}/bl2x.bin \
			--infile-dvinit-params=${payload}/device_acs.bin \
			--infile-csinit-params=${payload}/chip_acs.bin \
			--infile-ddr-fwdata=${payload}/ddrfw_data.bin \
			--outfile-bb1st=${output}/test-bb1st.bin \
			--outfile-blob-bl2e=${output}/test-blob-bl2e.bin \
			--outfile-blob-bl2x=${output}/test-blob-bl2x.bin

	if [ ! -f ${output}/test-bb1st.bin ] || \
	   [ ! -f ${output}/test-blob-bl2e.bin ] || \
	   [ ! -f ${output}/test-blob-bl2x.bin ]; then
		echo "Error: ${output}/ bootblob does not all exist... abort"
		ls -la ${output}/
		exit -1
	fi
	echo "done to genenrate test-bb1st.bin folder"
}

function mk_devfip() {
	output=$1
	payload=$2

	# fix size for BL30 128KB
	if [ -f ${output}/bl30.bin ]; then
		#blx_size=`du -b ${output}/bl30.bin | awk '{print int(${output}/bl30.bin)}'`
		blx_size=`stat -c %s ${output}/bl30.bin`
		if [ $blx_size -gt 131072 ]; then
			echo "Error: bl30 size exceed limit 131072"
			exit -1
		fi
	else
		echo "Warning: local bl30"
		#dd if=/dev/random of=${output}/bl30.bin bs=4096 count=1
		dd if=bl30/bin/sc2/bl30.bin of=${output}/bl30.bin
	fi
	dd if=/dev/zero of=${payload}/bl30.bin bs=131072 count=1
	dd if=${output}/bl30.bin of=${payload}/bl30.bin conv=notrunc

	# fix size for BL40 96KB
	if [ -f ${output}/bl40.bin ]; then
		#blx_size=`du -b ${output}/bl40.bin | awk '{print int(${output}/bl40.bin)}'`
		blx_szie=`stat -c %s ${output}/bl40.bin`
		if [ $blx_size -gt 98304 ]; then
			echo "Error: bl40 size exceed limit 98304"
			exit -1
		fi
	else
		echo "Warning: null bl40"
		#dd if=/dev/random of=${output}/bl40.bin bs=4096 count=1
		dd if=/dev/zero of=${output}/bl40.bin bs=4096 count=1
	fi
	dd if=/dev/zero of=${payload}/bl40.bin bs=98304 count=1
	dd if=${output}/bl40.bin of=${payload}/bl40.bin conv=notrunc


	# fix size for BL31 256KB
	if [ ! -f ${output}/bl31.bin ]; then
		echo "Error: ${output}/bl31.bin does not exist... abort"
		exit -1
	fi
	#blx_size=`du -b ${output}/bl31.bin | awk '{print int(${output}/bl31.bin)}'`
	blx_size=`stat -c %s ${output}/bl31.bin`
	echo "BL31 size: ${blx_size}"
	if [ $blx_size -gt 262144 ]; then
		echo "Error: bl31 size exceed limit 262144"
		exit -1
	fi
	dd if=/dev/zero of=${payload}/bl31.bin bs=262144 count=1
	dd if=${output}/bl31.bin of=${payload}/bl31.bin conv=notrunc


	# fix size for BL32 512KB
	if [ -f ${output}/bl32.bin ]; then
		#blx_size=`du -b ${output}/bl32.bin | awk '{print int(${output}/bl32.bin)}'`
		blx_size=`stat -c %s ${output}/bl32.bin`
		if [ $blx_size -gt 524288 ]; then
			echo "Error: bl32 size exceed limit 524288"
			exit -1
		fi
	else
		echo "Warning: local bl32"
		#dd if=/dev/random of=${output}/bl32.bin bs=4096 count=1
		dd if=bl32/bin/sc2/bl32.bin of=${output}/bl32.bin
	fi
	dd if=/dev/zero of=${payload}/bl32.bin bs=524288 count=1
	dd if=${output}/bl32.bin of=${payload}/bl32.bin conv=notrunc


	# fix size for BL33 1024KB
	if [ ! -f ${output}/bl33.bin ]; then
		echo "Error: ${output}/bl33.bin does not exist... abort"
		exit -1
	fi
	#blx_size=`du -b ${output}/bl33.bin | awk '{print int(${output}/bl33.bin)}'`
	blx_size=`stat -c %s ${output}/bl33.bin`
	if [ $blx_size -gt 1572864 ]; then
		echo "Error: bl33 size exceed limit 0x180000"
		exit -1
	fi
	dd if=/dev/zero of=${payload}/bl33.bin bs=1572864 count=1
	dd if=${output}/bl33.bin of=${payload}/bl33.bin conv=notrunc


	./${FIP_FOLDER}${CUR_SOC}/acpu-imagetool create-device-fip \
			--infile-bl30-payload=${payload}/bl30.bin \
			--infile-bl40-payload=${payload}/bl40.bin \
			--infile-bl31-payload=${payload}/bl31.bin \
			--infile-bl32-payload=${payload}/bl32.bin \
			--infile-bl33-payload=${payload}/bl33.bin \
			--outfile-device-fip=${output}/test-device-fip.bin

	if [ ! -f ${output}/test-device-fip.bin ]; then
		echo "Error: ${output}/test-device-fip.bin does not exist... abort"
		exit -1
	fi
	echo "done to genenrate test-device-fip.bin"
}

function mk_uboot() {
	output_images=$1
	input_payloads=$2

	device_fip="${input_payloads}/test-device-fip.bin"
	bb1st="${input_payloads}/test-bb1st.bin"
	bl2e="${input_payloads}/test-blob-bl2e.bin"
	bl2x="${input_payloads}/test-blob-bl2x.bin"

	if [ ! -f ${device_fip} ] || \
	   [ ! -f ${bb1st} ] || \
	   [ ! -f ${bl2e} ] || \
	   [ ! -f ${bl2x} ]; then
		echo "Error: ${input_payloads}/ bootblob does not all exist... abort"
		ls -la ${input_payloads}/
		exit -1
	fi

	file_info_cfg="${output_images}/aml-payload.cfg"
	file_info_cfg_temp=${temp_cfg}.temp
	bootloader="${output_images}/u-boot.bin"
	sdcard_image="${output_images}/u-boot.bin.sd.bin"

	#fake ddr fip 256KB
	ddr_fip="${input_payloads}/ddr-fip.bin"
	if [ ! -f ${ddr_fip} ]; then
		dd if=/dev/zero of=${ddr_fip} bs=1024 count=256 status=none
	fi

	#cat those together with 4K upper aligned for sdcard
	align_base=4096
	total_size=0
	for file in ${bb1st} ${bl2e} ${bl2x} ${ddr_fip} ${device_fip}; do
		size=`stat -c "%s" ${file}`
		upper=$[(size+align_base-1)/align_base*align_base]
		total_size=$[total_size+upper]
		echo ${file} ${size} ${upper}
	done

	echo ${total_size}
	rm -f ${bootloader}
	dd if=/dev/zero of=${bootloader} bs=${total_size} count=1 status=none

	sector=512
	seek=0
	seek_sector=0
	dateStamp=SC2-`date +%Y%m%d%H%M%S`

	echo @AMLBOOT > ${file_info_cfg_temp}
	dd if=${file_info_cfg_temp} of=${file_info_cfg} bs=1 count=8 conv=notrunc &> /dev/null
	nItemNum=5
	nSizeHDR=$[64+nItemNum*16]
	printf "01 %02x %02x %02x 00 00 00 00" $[(nItemNum)&0xFF] $[(nSizeHDR)&0xFF] $[((nSizeHDR)>>8)&0xFF] \
		| xxd -r -ps > ${file_info_cfg_temp}
	cat ${file_info_cfg_temp} >> ${file_info_cfg}

	echo ${dateStamp} > ${file_info_cfg_temp}
	dd if=${file_info_cfg_temp} of=${file_info_cfg} bs=1 count=16 oflag=append conv=notrunc &> /dev/null

	index=0
	arrPayload=("BBST" "BL2E" "BL2X" "DDRF" "DEVF");
	nPayloadOffset=0
	nPayloadSize=0
	for file in ${bb1st} ${bl2e} ${bl2x} ${ddr_fip} ${device_fip}; do
		size=`stat -c "%s" ${file}`
		size_sector=$[(size+align_base-1)/align_base*align_base]
		nPayloadSize=$[size_sector]
		size_sector=$[size_sector/sector]
		seek_sector=$[seek/sector+seek_sector]
		#nPayloadOffset=$[sector*(seek_sector+1)]
		nPayloadOffset=$[sector*(seek_sector)]
		echo ${file} ${seek_sector} ${size_sector}
		dd if=${file} of=${bootloader} bs=${sector} seek=${seek_sector} conv=notrunc status=none

		echo ${arrPayload[$index]} > ${file_info_cfg_temp}.x
		index=$((index+1))
		dd if=${file_info_cfg_temp}.x of=${file_info_cfg_temp} bs=1 count=4 &> /dev/null
		rm -f ${file_info_cfg_temp}.x
		printf "%02x %02x %02x %02x %02x %02x %02x %02x 00 00 00 00" $[(nPayloadOffset)&0xFF] $[((nPayloadOffset)>>8)&0xFF] $[((nPayloadOffset)>>16)&0xFF] $[((nPayloadOffset)>>24)&0xFF] \
		$[(nPayloadSize)&0xFF] $[((nPayloadSize)>>8)&0xFF] $[((nPayloadSize)>>16)&0xFF] $[((nPayloadSize)>>24)&0xFF] | xxd -r -ps >> ${file_info_cfg_temp}
		dd if=${file_info_cfg_temp} of=${file_info_cfg} oflag=append conv=notrunc &> /dev/null
		rm -f ${file_info_cfg_temp}
		seek=$[(size+align_base-1)/align_base*align_base]
	done

	openssl dgst -sha256 -binary ${file_info_cfg} > ${file_info_cfg}.sha256
	cat ${file_info_cfg} >> ${file_info_cfg}.sha256
	#cat ${file_info_cfg}.sha256 >> ${file_info_cfg}
	rm -f ${file_info_cfg}
	mv -f ${file_info_cfg}.sha256 ${file_info_cfg}

	dd if=${file_info_cfg} of=${bootloader} bs=512 seek=508 conv=notrunc status=none

	echo "Image SDCARD"
	total_size=$[total_size+512]
	rm -f ${sdcard_image}
	dd if=/dev/zero of=${sdcard_image} bs=${total_size} count=1 status=none
	dd if=${file_info_cfg}   of=${sdcard_image} conv=notrunc status=none
	dd if=${bootloader} of=${sdcard_image} bs=512 seek=1 conv=notrunc status=none

	rm -f ${file_info_cfg}
}

function cleanup() {
	cp ${FIP_BUILD_FOLDER}/test-* ${BUILD_FOLDER} -f
	echo "output file are generated in ${BUILD_FOLDER} folder"
	#rm -f ${BUILD_PATH}/test-*
	#rm -rf ${BUILD_PAYLOAD}
	rm -f ${BUILD_PATH}/bl*.enc ${BUILD_PATH}/bl2*.sig
}

function encrypt_step() {
	dbg "encrypt: $@"

}

function encrypt() {
	#u-boot.bin generate

	return
}

function build_fip() {

	# acs_tool process ddr timing and configurable parameters
	#python ${FIP_FOLDER}/acs_tool.pyc ${BUILD_PATH}/${AML_BL2_NAME} ${BUILD_PATH}/bl2_acs.bin ${BUILD_PATH}/acs.bin 0

	# fix bl2/bl2e/bl2x
	if [ -d ${BUILD_PAYLOAD} ]; then
		rm -rf ${BUILD_PAYLOAD}
	fi
	mkdir -p ${BUILD_PAYLOAD}/

	# make boot blobs
	mk_bl2ex ${BUILD_PATH} ${BUILD_PAYLOAD} ddr4

	# make devicefip
	mk_devfip ${BUILD_PATH} ${BUILD_PAYLOAD}


	# build final bootloader
	mk_uboot ${BUILD_PATH} ${BUILD_PATH}

	return
}

function copy_other_soc() {
	cp ${BL33_BUILD_FOLDER}${BOARD_DIR}/firmware/acs.bin ${BUILD_PATH}/device_acs.bin -f
	cp ${BL33_BUILD_FOLDER}${BOARD_DIR}/firmware/chip_acs.bin ${BUILD_PATH} -f

	# device acs params parse for ddr timing
	#./${FIP_FOLDER}parse ${BUILD_PATH}/device_acs.bin
}

function package() {
	# BUILD_PATH without "/"
	x=$((${#BUILD_PATH}-1))
	if [ "\\" == "${BUILD_PATH:$x:1}" ] || [ "/" == "${BUILD_PATH:$x:1}" ]; then
		BUILD_PATH=${BUILD_PATH:0:$x}
	fi

	init_vari $@
	build_fip $@

	#copy_file
	cleanup
	echo "Bootloader build done!"
}
