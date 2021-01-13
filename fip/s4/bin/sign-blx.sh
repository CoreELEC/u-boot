 #!/bin/bash

EXEC_BASEDIR=$(dirname $(readlink -f $0))
BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

#
# Settings
#
BASEDIR_BUILD="${BASEDIR_TOP}/output"
postfix=.signed
declare -a BLX_BIN_SIZE=("127904" "65536" "65536" "4096" "86016" "262144" "524288"  "98304")

function process_ddrfw() {
	local ddr_input=$1
	local ddr_output=$2
	local ddr_type=$3

	if [ "$ddr_type" == "ddr4" ]; then
		dd if=${ddr_input}/ddr4_1d.fw of=${ddr_output}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=${ddr_input}/ddr4_2d.fw of=${ddr_output}/ddrfw_2d.bin skip=96 bs=1 count=36864
	elif [ "$ddr_type" == "ddr3" ]; then
		dd if=${ddr_input}/ddr3_1d.fw of=${ddr_output}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=/dev/zero of=${ddr_output}/ddrfw_2d.bin bs=36864 count=1
	elif [ "$ddr_type" == "lpddr4" ]; then
		dd if=${ddr_input}/lpddr4_1d.fw of=${ddr_output}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=${ddr_input}/lpddr4_2d.fw of=${ddr_output}/ddrfw_2d.bin skip=96 bs=1 count=36864
	elif [ "$ddr_type" == "lpddr3" ]; then
		dd if=${ddr_input}/lpddr3_1d.fw of=${ddr_output}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=/dev/zero of=${ddr_output}/ddrfw_2d.bin bs=36864 count=1
	else
		echo "un-recognized ddr_type: ${ddr_type}"
		echo "---- use default ddr4 ----"
		dd if=${ddr_input}/ddr4_1d.fw of=${ddr_output}/ddrfw_1d.bin skip=96 bs=1 count=36864
		dd if=${ddr_input}/ddr4_2d.fw of=${ddr_output}/ddrfw_2d.bin skip=96 bs=1 count=36864
	fi

	piei_size=`stat -c %s ${ddr_input}/piei.fw`
	if [ $piei_size -gt 12384 ]; then
		dd if=${ddr_input}/piei.fw of=${ddr_output}/ddrfw_piei.bin skip=96 bs=1 count=12288
	else
		dd if=/dev/zero of=${ddr_output}/ddrfw_piei.bin bs=12288 count=1
		dd if=${ddr_input}/piei.fw of=${ddr_output}/ddrfw_piei.bin skip=96 bs=1 conv=notrunc
	fi

	cat ${ddr_output}/ddrfw_1d.bin ${ddr_output}/ddrfw_2d.bin \
		${ddr_output}/ddrfw_piei.bin > ${ddr_output}/ddr-fwdata.bin

	if [ ! -f ${ddr_output}/ddr-fwdata.bin ]; then
		echo "ddr-fwdata payload does not exist in ${ddr_output} !"
		exit -1
	fi
	ddrfw_data_size=`stat -c %s ${ddr_output}/ddr-fwdata.bin`
	if [ $ddrfw_data_size -ne 86016 ]; then
		echo "ddr-fwdata size is not equal to 84K, $ddrfw_data_size"
		exit -1
	fi
}
function sign_blx() {
    local argv=("$@")
    local i=0

     # Parse args

    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
				#echo "i=$i argv[$i]=${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --blxname)
               blxname="${argv[$i]}" ;;
            --input)
               input="${argv[$i]}" ;;
            --output)
                output="${argv[$i]}" ;;
			--ddr_type)
                ddr_type="${argv[$i]}" ;;
			--chip_acs)
                chip_acs="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done

	if [ ! -f ${input} ]; then
		echo "input ${input} invalid"
		exit 1
	fi

	mkdir ${BASEDIR_BUILD}

	if [ ${blxname} == "bl2" ] || [ ${blxname} == "bl2e" ] || [ ${blxname} == "bl2x" ]; then
		dd if=/dev/zero of=${BASEDIR_BUILD}/bl2-payload.bin bs=${BLX_BIN_SIZE[0]} count=1
		dd if=/dev/zero of=${BASEDIR_BUILD}/bl2e-payload.bin bs=${BLX_BIN_SIZE[1]} count=1
		dd if=/dev/zero of=${BASEDIR_BUILD}/bl2x-payload.bin bs=${BLX_BIN_SIZE[2]} count=1
		dd if=/dev/zero of=${BASEDIR_BUILD}/csinit-params.bin bs=${BLX_BIN_SIZE[3]} count=1
		dd if=/dev/zero of=${BASEDIR_BUILD}/ddr-fwdata.bin bs=${BLX_BIN_SIZE[4]} count=1
	elif [ ${blxname} == "bl31" ]; then
		dd if=/dev/zero of=${BASEDIR_BUILD}/${blxname}-payload.bin bs=${BLX_BIN_SIZE[5]} count=1
	elif [ ${blxname} == "bl32" ]; then
		dd if=/dev/zero of=${BASEDIR_BUILD}/${blxname}-payload.bin bs=${BLX_BIN_SIZE[6]} count=1
	elif [ ${blxname} == "bl40" ]; then
		dd if=/dev/zero of=${BASEDIR_BUILD}/${blxname}-payload.bin bs=${BLX_BIN_SIZE[7]} count=1
	else
		echo invalid blxname [$blxname]
		exit 1
	fi

	${EXEC_BASEDIR}/download-keys.sh chipset

	if [ ${blxname} == "bl2" ]; then
		if [ -z ${chip_acs} ] || [ ! -f ${chip_acs} ]; then
			echo "input ${chip_acs} invalid"
			exit 1
		fi
		dd if=${chip_acs} of=${BASEDIR_BUILD}/csinit-params.bin conv=notrunc
		dd if=${input} of=${BASEDIR_BUILD}/${blxname}-payload.bin conv=notrunc
		process_ddrfw ${BASEDIR_TOP} ${BASEDIR_BUILD} ${ddr_type}
		${EXEC_BASEDIR}/gen-boot-blobs.sh ${BASEDIR_BUILD} ${BASEDIR_BUILD}
	elif [ ${blxname} == "bl2e" ] || [ ${blxname} == "bl2x" ]; then
		dd if=${input} of=${BASEDIR_BUILD}/${blxname}-payload.bin conv=notrunc
		${EXEC_BASEDIR}/gen-boot-blobs.sh ${BASEDIR_BUILD} ${BASEDIR_BUILD}
	elif [ ${blxname} == "bl31" ] || [ ${blxname} == "bl32" ] || [ ${blxname} == "bl40" ]; then
		dd if=${input} of=${BASEDIR_BUILD}/${blxname}-payload.bin conv=notrunc
		${EXEC_BASEDIR}/gen-bl3x-blobs.sh ${blxname:2:2} ${BASEDIR_BUILD} ${BASEDIR_BUILD}
	fi

	if [ ${blxname} == "bl2" ]; then
		cp ${BASEDIR_BUILD}/bb1st.bin${postfix} $output
	else
		cp ${BASEDIR_BUILD}/blob-${blxname}.bin${postfix} $output
	fi
}

rm -rf ${BASEDIR_BUILD}
sign_blx $@
rm -rf ${BASEDIR_BUILD}
