#!/bin/bash

set -e
#set -x

version=3.1
dbg_trace=1

# Provides chip_die_list,
#source ${BASEDIR_TOP}/common/util.sh

declare -A sig_scheme_list
sig_scheme_list["rsa"]="rsa"
sig_scheme_list["mldsa"]="mldsa"
sig_scheme_list["rsa-mldsa"]="rsa-mldsa"

declare -A ml_dsa_level_list
ml_dsa_level_list["2"]="2"
ml_dsa_level_list["3"]="3"
ml_dsa_level_list["5"]="5"

declare -A ml_dsa_version_list
ml_dsa_version_list["draft1"]="draft1"
#ml_dsa_version["final"]="final"

declare -A ops_list
ops_list["create-boot-blobs"]="create-boot-blobs"
ops_list["create-device-fip"]="create-device-fip"

declare -A fip_header_layout_list
fip_header_layout_list["full"]="full"
fip_header_layout_list["compact"]="compact"
fip_header_layout_list["mini"]="mini"

#
# Utilities
#

trace ()
{
	if [ ${dbg_trace:-1} -ne 0 ];
	then
		echo ">>> $@"
		#echo ">>> $@" > /dev/null
	fi
}

check_file() {
	if [ ! -f "$1" ]; then echo "Error: file \""$1"\" does NOT exist"; usage ; fi
}

check_dir() {
	if [ ! -d "$1" ]; then echo "Error: directory \""$1"\" does NOT exist"; usage ; fi
}

check_value() {
	local val=$1
	local begin=$2
	local end=$3

	if [ $val -lt $begin ] || [ $val -gt $end ]; then
		echo "Error: Value $val is not in range [$begin, $end]"
		exit 1
	fi
}

function get_mldsa_authen_algo {
    local __resultvar=$1
    local scheme=$2
    local version=$3

    local tmp=${sig_scheme_list[$scheme]}
    if [ "${tmp}" == "" ]; then
        echo "Error: Incorrect signature scheme: ${scheme}"
        return -1
    elif [ ${tmp} == "mldsa" ] || [ ${tmp} == "rsa-mldsa" ]; then
	if [ -z "$version" ]; then
            echo "Error: Missing ML-DSA version"
	    return -1
	fi

	# FIXME: Hardcoded to "draft1"
        local tmp2="draft1"
        #local tmp2=${ml_dsa_version_list[$version]}
	#if [ "${tmp2}" == "" ]; then
        #    echo "Error: Invalid ML-DSA version $version"
        #    return -1
	#fi

        eval $__resultvar="mldsa-${tmp2}"
	return 0
    else
        eval $__resultvar="none"
        return 0
    fi
}

usage() {
	cat << EOF
Usage: $(basename $0) --help | --version

		Generate Amlogic SC2 Secure Chipset Startup (SCS) root hash and certificate template

		$(basename $0)
		--payload-dir <payload-dir> \\
		--rootkey-index [0 | 1 | 2 | 3] \\
		--key-dir <key-dir-prefix> \\
		--ops [create-boot-blobs | create-device-fip] \\
		{--trust-chain [chipset | device-vendor]} \\
		{--scs-family [s4 | sc2 | p1 | s5]} \\
		{--project <project-name>} \\
		{--output-dir <output-dir>} \\
		{--sig-scheme [rsa | mldsa | rsa-mldsa]} \\
		{--template-layout [rsa | mldsa | rsa-mldsa]} \\
		{--ml-dsa-version [draft1]}
EOF
	exit 1
}

parse_main() {
	local i=0
	local argv=()
	for arg in "$@" ; do
		argv[$i]="$arg"
		i=$((i + 1))
	done

	i=0
	while [ $i -lt $# ]; do
		arg="${argv[$i]}"
		i=$((i + 1))
		case "$arg" in
			-h|--help)
				usage
				break
				;;
			-v|--version)
				echo "Version $version";
		exit 0
		;;
	--project)
		project="${argv[$i]}"
		;;
	--device-soc)
		device_soc="${argv[$i]}"
		;;
	--key-dir)
		key_dir="${argv[$i]}"
		check_dir "${key_dir}"
		;;
	--payload-dir)
		payload_dir="${argv[$i]}"
		check_dir "${payload_dir}"
		;;
	--rootkey-index)
		rootkey_index="${argv[$i]}"
		check_value $rootkey_index 0 3
		;;
	--output-dir)
		output_dir="${argv[$i]}"
		;;
	--trust-chain)
		trust_chain="${argv[$i]}"
		;;
	--output-file-ext)
		output_file_ext=".${argv[$i]}"
		;;
	--no-encryption)
		with_encryption=0;
		i=$((i - 1))
		;;
	--no-root-hash)
		write_root_hash=0;
		i=$((i - 1))
		;;
	--no-bb1st-template)
		write_bb1st_template=0;
		i=$((i - 1))
		;;
	--scs-family)
		scs_family="${argv[$i]}"
		#
		# Skip parameter validation here and let acpu-imagetool validate the scs-family input
		#

		#if [ ${scs_family} != "s4" ]; then
		#    echo "Error: Invalid SCS family ${scs_family}"
		#    usage
		#fi
		;;
	--sig-scheme)
		sig_scheme="${argv[$i]}"
		;;
	--template-dir)
		template_dir="${argv[$i]}"
		check_dir "${template_dir}"
		;;
	--template-layout)
		template_layout="${argv[$i]}"
		;;
	--ml-dsa-version)
		ml_dsa_version="${argv[$i]}"
		;;
	--ops)
		OPS="${argv[$i]}"
		;;
	--compact-header-layout)
		FIP_HEADER_LAYOUT="full"
		i=$((i - 1))
		;;
	--fip-header-layout)
		FIP_HEADER_LAYOUT="${argv[$i]}"
		;;
	--device-scs-segid)
		DEVICE_SCS_SEGID="${argv[$i]}"
		;;
	--device-vendor-segid)
		DEVICE_VENDOR_SEGID="${argv[$i]}"
		;;
	--device-tee-vers)
		DEVICE_TEE_VERS="${argv[$i]}"
		;;
	--device-ree-vers)
		DEVICE_REE_VERS="${argv[$i]}"
		;;
	--device-scs-vers)
		DEVICE_SCS_VERS="${argv[$i]}"
		;;
	--device-lvl1cert-vers-submask)
		DEVICE_SCS_LVL1CERT_VERS_SUBMASK="${argv[$i]}"
		;;
	*)
		echo "Unknown option $arg";
		usage
		;;
	esac
	i=$((i + 1))
	done
}

#
# Variables
#

OPS=""
FIP_HEADER_LAYOUT="full"
PREFIX="cs-"
EXEC_BASEDIR=$(dirname $(readlink -f $0))
if [ -z "${ACPU_IMAGETOOL}" ]; then
	ACPU_IMAGETOOL=${EXEC_BASEDIR}/acpu-imagetool
	if [ ! -f "$ACPU_IMAGETOOL" ]; then
		ACPU_IMAGETOOL=${EXEC_BASEDIR}/../../binary-tool/acpu-imagetool
	fi
fi

key_dir=""
payload_dir=""
rootkey_index=0
output_dir=""
project=""
device_soc=""
trust_chain=""
scs_family=""
output_file_ext=""
with_encryption=1
write_root_hash=1
write_bb1st_template=1
sig_scheme=""
sig_scheme_version=""
template_dir=""
template_layout=""
DEVICE_SCS_SEGID=0x0
DEVICE_VENDOR_SEGID=0x0
DEVICE_TEE_VERS=0x0
DEVICE_REE_VERS=0x0
DEVICE_SCS_VERS=0x0

# Change root trust chain name from "rootrsa" to "trustchain"
trustchain_name="trustchain"

is_rsa=1
is_ml_dsa=0
is_hybrid=0

rsa_algo_name="rsa"
ml_dsa_level=""
ml_dsa_algo_name="mldsa"
ml_dsa_version=""

COMMON_CREATE_BOOT_BLOBS_ARGS=""
COMMON_CREATE_BOOT_BLOBS_CS_ARGS=""
COMMON_CREATE_BOOT_BLOBS_DV_ARGS=""
RSA_CREATE_BOOT_BLOBS_ARGS=""
RSA_CREATE_BOOT_BLOBS_CS_ARGS=""
RSA_CREATE_BOOT_BLOBS_DV_ARGS=""
PQC_CREATE_BOOT_BLOBS_ARGS=""
PQC_CREATE_BOOT_BLOBS_CS_ARGS=""
PQC_CREATE_BOOT_BLOBS_DV_ARGS=""

COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST=""
RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST=""
PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST=""

COMMON_CREATE_DEVICE_FIP_ARGS=""
COMMON_CREATE_DEVICE_FIP_CS_ARGS=""
COMMON_CREATE_DEVICE_FIP_DV_ARGS=""
RSA_CREATE_DEVICE_FIP_ARGS=""
RSA_CREATE_DEVICE_FIP_CS_ARGS=""
RSA_CREATE_DEVICE_FIP_DV_ARGS=""
PQC_CREATE_DEVICE_FIP_ARGS=""
PQC_CREATE_DEVICE_FIP_CS_ARGS=""
PQC_CREATE_DEVICE_FIP_DV_ARGS=""

COMMON_CREATE_DEVICE_FIP_FILE_CHECKLIST=""
RSA_CREATE_DEVICE_FIP_FILE_CHECKLIST=""
PQC_CREATE_DEVICE_FIP_FILE_CHECKLIST=""

trace "======== $0"

parse_main "$@"

if [ -z "$key_dir" ]; then
	key_dir="."
fi

if [ -z "$trust_chain" ]; then
	trust_chain="chipset"
fi

if [ ${trust_chain^^} != "CHIPSET" ] && [ ${trust_chain^^} != "DEVICE-VENDOR" ]; then
	echo "Error: Invalid trust chain $trust_chain"
	usage
fi

if [ ${trust_chain^^} == "CHIPSET" ]; then
	PREFIX="cs-"
	PREFIX_ARG="chipset"
	PREFIX_BS="cs-"
else
	PREFIX=""
	PREFIX_ARG="device"
	PREFIX_BS="device-"
fi

if [ -z "$ml_dsa_level" ]; then
	ml_dsa_level=3
fi

tmp=${ops_list[$OPS]}
if [ "${tmp}" == "" ]; then
	echo "Error: Invalid OPS $OPS"
	usage
fi
OPS=$tmp

tmp=${fip_header_layout_list[${FIP_HEADER_LAYOUT}]}
if [ "${tmp}" == "" ]; then
	echo "Error: Invalid FIP header layout $tmp"
	usage
fi
FIP_HEADER_LAYOUT=$tmp

if [ -z "$sig_scheme" ]; then
	sig_scheme="rsa"
fi

tmp=${sig_scheme_list[$sig_scheme]}
if [ "${tmp}" == "" ]; then
	echo "Error: Invalid signature scheme $sig_scheme"
	usage
fi

sig_scheme=$tmp
if [ ${sig_scheme} == "mldsa" ] || [ ${sig_scheme} == "rsa-mldsa" ]; then
	if [ -z "$ml_dsa_version" ]; then
		echo "Error: Missing ML-DSA version"
		usage
	fi

	tmp=${ml_dsa_level_list[$ml_dsa_level]}
	if [ "$tmp" == "" ]; then
		echo "Error: Invalid ML-DSA key level $ml_dsa_level"
		usage
	fi

	tmp=${ml_dsa_version_list[$ml_dsa_version]}
	if [ "$tmp" == "" ]; then
		echo "Error: Invalid ML-DSA version $ml_dsa_version"
		usage
	fi
fi

sig_scheme_version=$sig_scheme
if [ ${sig_scheme} == "rsa" ]; then
	is_rsa=1
	is_ml_dsa=0
	is_hybrid=0
	if [ ${trust_chain^^} == "CHIPSET" ]; then
		COMMON_CREATE_BOOT_BLOBS_ARGS="--chipset-authen-algorithm=${rsa_algo_name},none"
		COMMON_CREATE_DEVICE_FIP_ARGS="--chipset-authen-algorithm=${rsa_algo_name},none"
	else
		COMMON_CREATE_BOOT_BLOBS_ARGS="--device-authen-algorithm=${rsa_algo_name},none"
		COMMON_CREATE_DEVICE_FIP_ARGS="--device-authen-algorithm=${rsa_algo_name},none"
	fi
fi
if [ ${sig_scheme} == "mldsa" ]; then
	is_rsa=0
	is_ml_dsa=1
	is_hybrid=0
	if [ "${ml_dsa_version}" != "final" ]; then
		sig_scheme_version=${sig_scheme}-${ml_dsa_version}
		ml_dsa_algo_name=${ml_dsa_algo_name}-${ml_dsa_version}
	fi
	if [ ${trust_chain^^} == "CHIPSET" ]; then
		COMMON_CREATE_BOOT_BLOBS_ARGS="--chipset-authen-algorithm=none,${ml_dsa_algo_name}"
		COMMON_CREATE_DEVICE_FIP_ARGS="--chipset-authen-algorithm=none,${ml_dsa_algo_name}"
	else
		COMMON_CREATE_BOOT_BLOBS_ARGS="--device-authen-algorithm=none,${ml_dsa_algo_name}"
		COMMON_CREATE_DEVICE_FIP_ARGS="--device-authen-algorithm=none,${ml_dsa_algo_name}"
	fi
fi
if [ ${sig_scheme} == "rsa-mldsa" ]; then
	is_rsa=1
	is_ml_dsa=1
	is_hybrid=1
	if [ "${ml_dsa_version}" != "final" ]; then
		sig_scheme_version=${sig_scheme}-${ml_dsa_version}
		ml_dsa_algo_name=${ml_dsa_algo_name}-${ml_dsa_version}
	fi
	if [ ${trust_chain^^} == "CHIPSET" ]; then
		COMMON_CREATE_BOOT_BLOBS_ARGS="--chipset-authen-algorithm=${rsa_algo_name},${ml_dsa_algo_name}"
		COMMON_CREATE_DEVICE_FIP_ARGS="--chipset-authen-algorithm=${rsa_algo_name},${ml_dsa_algo_name}"
	else
		COMMON_CREATE_BOOT_BLOBS_ARGS="--device-authen-algorithm=${rsa_algo_name},${ml_dsa_algo_name}"
		COMMON_CREATE_DEVICE_FIP_ARGS="--device-authen-algorithm=${rsa_algo_name},${ml_dsa_algo_name}"
	fi
fi

if [ ! -z "${__ENV_TEMPLATE_LAYOUT}" ]; then
    echo "!!!!!!!! Using __ENV_TEMPLATE_LAYOUT ${__ENV_TEMPLATE_LAYOUT}"
    template_layout=${__ENV_TEMPLATE_LAYOUT}
fi

if [ ! -z "${template_layout}" ]; then
    trace "  template-layout ${template_layout}"
    get_mldsa_authen_algo mldsa_authen_algo $template_layout $ml_dsa_version
    if [ $? -ne 0 ]; then
        usage
    fi
    trace "  mldsa_authen_algo ${mldsa_authen_algo}"

    if [ ${trust_chain^^} == "CHIPSET" ]; then
        COMMON_CREATE_BOOT_BLOBS_ARGS+=" --device-authen-algorithm=${rsa_algo_name},${mldsa_authen_algo}"
        COMMON_CREATE_DEVICE_FIP_ARGS+=" --device-authen-algorithm=${rsa_algo_name},${mldsa_authen_algo}"
	else
        COMMON_CREATE_BOOT_BLOBS_ARGS+=" --chipset-authen-algorithm=${rsa_algo_name},${mldsa_authen_algo}"
        COMMON_CREATE_DEVICE_FIP_ARGS+=" --chipset-authen-algorithm=${rsa_algo_name},${mldsa_authen_algo}"
    fi
else
    if [ ${trust_chain^^} == "CHIPSET" ]; then
        COMMON_CREATE_BOOT_BLOBS_ARGS+=" --device-authen-algorithm=${rsa_algo_name},none"
        COMMON_CREATE_DEVICE_FIP_ARGS+=" --device-authen-algorithm=${rsa_algo_name},none"
    fi
fi

# FIXME: Hard code to "draft1"
# TODO: Is SOC die passed down during template and signing operation?
if [ ${trust_chain^^} == "DEVICE-VENDOR" ]; then
	if [ "$device_soc" == "s6" ]; then
		#FIXME cs_sig_scheme should be based on template-layout which would indicate CS scheme
		cs_sig_scheme="${template_layout:-rsa-mldsa}"
		template_ext=".$sig_scheme.$cs_sig_scheme"
	elif [ "$device_soc" == "s7d" ]; then
		cs_sig_scheme="${template_layout:-rsa}"
		template_ext=".$sig_scheme.$cs_sig_scheme"
	else
		template_ext=""
	fi
fi

#trace " --> $COMMON_CREATE_BOOT_BLOBS_ARGS"
trace "       sig-scheme $sig_scheme"
trace "sig-scheme-version $sig_scheme_version"
trace "     ml-dsa-level $ml_dsa_level"
trace "   ml-dsa-version $ml_dsa_version"
trace "              ops $OPS"
trace "      trust-chain $trust_chain"
trace "FIP_HEADER_LAYOUT ${FIP_HEADER_LAYOUT}"
trace "   ACPU_IMAGETOOL ${ACPU_IMAGETOOL}"
trace "  template-layout ${template_layout}"
trace "  output_file_ext ${output_file_ext}"
if [ ${trust_chain^^} == "DEVICE-VENDOR" ]; then
trace "DEVICE_SCS_SEGID ${DEVICE_SCS_SEGID}"
trace "DEVICE_VENDOR_SEGID ${DEVICE_VENDOR_SEGID}"
trace "DEVICE_TEE_VERS   ${DEVICE_TEE_VERS}"
trace "DEVICE_REE_VERS   ${DEVICE_REE_VERS}"
trace "DEVICE_SCS_VERS   ${DEVICE_SCS_VERS}"
trace "DEVICE_SCS_LVL1CERT_VERS_SUBMASK ${DEVICE_SCS_LVL1CERT_VERS_SUBMASK}"
fi

#
# Settings
#

BASEDIR_ROOT=${key_dir}
BASEDIR_PAYLOAD="${payload_dir}"

ROOTRSA_INDEX=${rootkey_index}

if [ -z "$project" ]; then
	BASEDIR_AESKEY_ROOT="${BASEDIR_ROOT}/root/aes/rootkey"
	BASEDIR_RSAKEY_ROOT="${BASEDIR_ROOT}/root/${sig_scheme}/"
	BASEDIR_BOOTBLOBS_RSAKEY_ROOT="${BASEDIR_ROOT}/boot-blobs/${sig_scheme}/${trustchain_name}-${ROOTRSA_INDEX}"
	#BASEDIR_BOOTBLOBS_AESKEY_ROOT="${BASEDIR_ROOT}/boot-blobs/aes/${trustchain_name}-${ROOTRSA_INDEX}/protkey"
	BASEDIR_FIP_RSAKEY_ROOT="${BASEDIR_ROOT}/fip/${sig_scheme}/${trustchain_name}-${ROOTRSA_INDEX}"

	BASEDIR_ROOTHASH_OUTPUT="${BASEDIR_RSAKEY_ROOT}/roothash"
	BASEDIR_BOOTBLOBS_PROTKEY_OUTPUT="${BASEDIR_ROOT}/boot-blobs/aes/${trustchain_name}-${ROOTRSA_INDEX}/protkey"
	BASEDIR_BOOTBLOBS_TEMPLATE_OUTPUT="${BASEDIR_ROOT}/boot-blobs/template/${trustchain_name}-${ROOTRSA_INDEX}"
	BASEDIR_FIP_TEMPLATE_OUTPUT="${BASEDIR_ROOT}/fip/template/${trustchain_name}-${ROOTRSA_INDEX}"
	BASEDIR_FIP_PROTKEY_OUTPUT="${BASEDIR_ROOT}/fip/aes/${trustchain_name}-${ROOTRSA_INDEX}/protkey"
else
	BASEDIR_AESKEY_ROOT="${BASEDIR_ROOT}/root/aes/${project}/rootkey"
	BASEDIR_RSAKEY_ROOT="${BASEDIR_ROOT}/root/${sig_scheme}/${project}"
	BASEDIR_BOOTBLOBS_RSAKEY_ROOT="${BASEDIR_ROOT}/boot-blobs/${sig_scheme}/${project}/${trustchain_name}-${ROOTRSA_INDEX}"
	#BASEDIR_BOOTBLOBS_AESKEY_ROOT="${BASEDIR_ROOT}/boot-blobs/aes/${project}/${trustchain_name}-${ROOTRSA_INDEX}/protkey"
	BASEDIR_FIP_RSAKEY_ROOT="${BASEDIR_ROOT}/fip/${sig_scheme}/${project}/${trustchain_name}-${ROOTRSA_INDEX}"

	BASEDIR_ROOTHASH_OUTPUT="${BASEDIR_RSAKEY_ROOT}/roothash"
	BASEDIR_BOOTBLOBS_PROTKEY_OUTPUT="${BASEDIR_ROOT}/boot-blobs/aes/${project}/${trustchain_name}-${ROOTRSA_INDEX}/protkey"
	BASEDIR_BOOTBLOBS_TEMPLATE_OUTPUT="${BASEDIR_ROOT}/boot-blobs/template/${project}/${trustchain_name}-${ROOTRSA_INDEX}"
	BASEDIR_FIP_TEMPLATE_OUTPUT="${BASEDIR_ROOT}/fip/template/${project}/${trustchain_name}-${ROOTRSA_INDEX}"
	BASEDIR_FIP_PROTKEY_OUTPUT="${BASEDIR_ROOT}/fip/aes/${project}/${trustchain_name}-${ROOTRSA_INDEX}/protkey"
fi

if [ -z "$output_dir" ]; then
	BASEDIR_OUTPUT_HASH="${BASEDIR_ROOTHASH_OUTPUT}"
	BASEDIR_OUTPUT_BLOB="${BASEDIR_BOOTBLOBS_TEMPLATE_OUTPUT}"
	BASEDIR_OUTPUT_PROTKEY="${BASEDIR_BOOTBLOBS_PROTKEY_OUTPUT}"
	BASEDIR_FIP_OUTPUT_BLOB="${BASEDIR_FIP_TEMPLATE_OUTPUT}"
	BASEDIR_FIP_OUTPUT_PROTKEY="${BASEDIR_FIP_PROTKEY_OUTPUT}"

	mkdir -p ${BASEDIR_OUTPUT_HASH}
	if [ ${write_bb1st_template} -eq 1 ]; then
		mkdir -p ${BASEDIR_OUTPUT_BLOB}
	fi

	# Protkey output
	if [ ${trust_chain^^} == "CHIPSET" ]; then
		mkdir -p ${BASEDIR_OUTPUT_PROTKEY}
	fi

	# FIP output
	mkdir -p ${BASEDIR_FIP_OUTPUT_BLOB}
	mkdir -p ${BASEDIR_FIP_OUTPUT_PROTKEY}
else
	check_dir "${output_dir}"
	BASEDIR_OUTPUT_HASH="${output_dir}"
	BASEDIR_OUTPUT_BLOB="${output_dir}"

	# Protkey output
	BASEDIR_OUTPUT_PROTKEY="${output_dir}"

	# FIP output
	BASEDIR_FIP_OUTPUT_BLOB="${output_dir}"
	BASEDIR_FIP_OUTPUT_PROTKEY="${output_dir}"
fi

#BASEDIR_OUTPUT_BLOB="./output/blob"
#BASEDIR_OUTPUT_HASH="./output/hash"
#BASEDIR_OUTPUT_PROTKEY="./output/protkey"

#
# Check inputs
#

check_dir "${BASEDIR_ROOT}"
if [ ${with_encryption} -eq 1 ]; then
	check_dir "${BASEDIR_AESKEY_ROOT}"
fi
check_dir "${BASEDIR_RSAKEY_ROOT}"
check_dir "${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}"
#check_dir "${BASEDIR_BOOTBLOBS_AESKEY_ROOT}"
#check_dir "${BASEDIR_FIP_RSAKEY_ROOT}"
#check_dir "${BASEDIR_PAYLOAD}"

#check_file "${BASEDIR_TEMPLATE}/chipset/bb1st.bin"
RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-0-pub.pem"
RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-1-pub.pem"
RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-2-pub.pem"
RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-3-pub.pem"
#check_file "${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-0-pub.pem"
#check_file "${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-1-pub.pem"
#check_file "${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-2-pub.pem"
#check_file "${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-3-pub.pem"

PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-0-pub.pem"
PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-1-pub.pem"
PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-2-pub.pem"
PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-3-pub.pem"

COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/epk/${PREFIX}rootcert-epks.bin"
COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/nonce/${PREFIX}rootkey-${ROOTRSA_INDEX}-nonce.bin"
#check_file "${BASEDIR_RSAKEY_ROOT}/epk/${PREFIX}rootcert-epks.bin"
#check_file "${BASEDIR_RSAKEY_ROOT}/nonce/${PREFIX}rootkey-${ROOTRSA_INDEX}-nonce.bin"

RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-${ROOTRSA_INDEX}-priv.pem"
RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${rsa_algo_name}-pub.pem"
#check_file "${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-${ROOTRSA_INDEX}-priv.pem"
#check_file "${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${rsa_algo_name}-pub.pem"

PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-${ROOTRSA_INDEX}-priv.pem"
PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${ml_dsa_algo_name}-pub.pem"

COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl1cert-epks.bin"
COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/nonce/${PREFIX}lvl1key-nonce.bin"
#check_file "${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl1cert-epks.bin"
#check_file "${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/nonce/${PREFIX}lvl1key-nonce.bin"

RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${rsa_algo_name}-priv.pem"
RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${rsa_algo_name}-pub.pem"
#check_file "${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${rsa_algo_name}-priv.pem"
#check_file "${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${rsa_algo_name}-pub.pem"

PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${ml_dsa_algo_name}-priv.pem"
PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${ml_dsa_algo_name}-pub.pem"

COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl2cert-epks.bin"
COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/nonce/${PREFIX}lvl2key-nonce.bin"
#check_file "${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl2cert-epks.bin"
#check_file "${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/nonce/${PREFIX}lvl2key-nonce.bin"

RSA_CREATE_DEVICE_FIP_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${rsa_algo_name}-priv.pem"
#check_file "${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${rsa_algo_name}-priv.pem"

if [ ${with_encryption} -eq 1 ]; then
	COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX_BS}rootkey-bootstage-2.bin"
	COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST+=" ${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX_BS}rootkey-bootstage-3.bin"
	#check_file "${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX}rootkey-bootstage-2.bin"
	#check_file "${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX}rootkey-bootstage-3.bin"

	COMMON_CREATE_DEVICE_FIP_FILE_CHECKLIST+=" ${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl2cert-epks.bin"
	COMMON_CREATE_DEVICE_FIP_FILE_CHECKLIST+=" ${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX_BS}rootkey-bootstage-1.bin"
	#check_file "${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX}rootkey-bootstage-1.bin"
fi

#
# Arguments
#

BB1ST_ARGS="${BB1ST_ARGS}"

#
# Select chipset family
#
if [ "${scs_family}" != "" ]; then
	COMMON_CREATE_BOOT_BLOBS_ARGS+=" --scs-family=${scs_family}" 
	#BB1ST_ARGS="${BB1ST_ARGS} --scs-family=${scs_family}"
fi

COMMON_CREATE_DEVICE_FIP_ARGS+=" --header-layout=${FIP_HEADER_LAYOUT}"
COMMON_CREATE_DEVICE_FIP_ARGS+=" --size-payload-bl30=90112"

### Input: payloads ###
#BB1ST_ARGS="${BB1ST_ARGS} --infile-bl2-payload=${BASEDIR_PAYLOAD}/bl2-payload.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-bl2e-payload=${BASEDIR_PAYLOAD}/bl2e-payload.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-bl2x-payload=${BASEDIR_PAYLOAD}/bl2x-payload.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-dvinit-params=${BASEDIR_PAYLOAD}/dvinit-params.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-csinit-params=${BASEDIR_PAYLOAD}/csinit-params.bin"

COMMON_CREATE_BOOT_BLOBS_CS_ARGS+=" --switch-chipset-sign-bl2=0"
COMMON_CREATE_BOOT_BLOBS_CS_ARGS+=" --switch-chipset-sign-blob=0"
#BB1ST_ARGS="${BB1ST_ARGS} --switch-chipset-sign-bl2=0"
#BB1ST_ARGS="${BB1ST_ARGS} --switch-chipset-sign-blob=0"

#if [ "${scs_family}" == "" ]; then
	# non-S4 chipset family needs to specify ddr-fwdata
	#BB1ST_ARGS="${BB1ST_ARGS} --infile-ddr-fwdata=${BASEDIR_PAYLOAD}/ddr-fwdata.bin"
#fi

#BB1ST_ARGS="${BB1ST_ARGS} --infile-bl30-payload=${BASEDIR_PAYLOAD}/bl30-payload.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-bl40-payload=${BASEDIR_PAYLOAD}/bl40-payload.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-bl31-payload=${BASEDIR_PAYLOAD}/bl31-payload.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-bl32-payload=${BASEDIR_PAYLOAD}/bl32-payload.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-bl33-payload=${BASEDIR_PAYLOAD}/bl33-payload.bin"

### Input: template ###
#BB1ST_ARGS="${BB1ST_ARGS} --infile-template-bb1st=${BASEDIR_TEMPLATE}/${PREFIX_ARG}/bb1st.bin"
COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --infile-template-bb1st=${template_dir}/$project/bb1st.bin${template_ext}"
COMMON_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-template-chipset-fip-header=${template_dir}/$project/device-fip-header.bin${template_ext}"

### Input: RootCert ###
RSA_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-rootrsa-0=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-0-pub.pem"
RSA_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-rootrsa-1=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-1-pub.pem"
RSA_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-rootrsa-2=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-2-pub.pem"
RSA_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-rootrsa-3=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-3-pub.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-pubkey-${PREFIX_ARG}-rootrsa-0=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-0-pub.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-pubkey-${PREFIX_ARG}-rootrsa-1=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-1-pub.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-pubkey-${PREFIX_ARG}-rootrsa-2=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-2-pub.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-pubkey-${PREFIX_ARG}-rootrsa-3=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-3-pub.pem"
PQC_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-root-0-pqc=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-0-pub.pem"
PQC_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-root-1-pqc=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-1-pub.pem"
PQC_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-root-2-pqc=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-2-pub.pem"
PQC_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-root-3-pqc=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-3-pub.pem"

### RootCert EK and NONCE
# EK is common for all root RSA
# NONCE is per root RSA
COMMON_CREATE_BOOT_BLOBS_ARGS+=" --infile-epks-${PREFIX_ARG}-rootcert=${BASEDIR_RSAKEY_ROOT}/epk/${PREFIX}rootcert-epks.bin"
COMMON_CREATE_BOOT_BLOBS_ARGS+=" --infile-nonce-${PREFIX_ARG}-rootrsa=${BASEDIR_RSAKEY_ROOT}/nonce/${PREFIX}rootkey-${ROOTRSA_INDEX}-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-epks-${PREFIX_ARG}-rootcert=${BASEDIR_RSAKEY_ROOT}/epk/${PREFIX}rootcert-epks.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-${PREFIX_ARG}-rootrsa=${BASEDIR_RSAKEY_ROOT}/nonce/${PREFIX}rootkey-${ROOTRSA_INDEX}-nonce.bin"

# Select root RSA to use
COMMON_CREATE_BOOT_BLOBS_ARGS+=" --${PREFIX_ARG}-rootrsa-index=${ROOTRSA_INDEX}"
#BB1ST_ARGS="${BB1ST_ARGS} --${PREFIX_ARG}-rootrsa-index=${ROOTRSA_INDEX}"

RSA_CREATE_BOOT_BLOBS_ARGS+=" --infile-signkey-${PREFIX_ARG}-root=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-${ROOTRSA_INDEX}-priv.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-signkey-${PREFIX_ARG}-root=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${rsa_algo_name}-${ROOTRSA_INDEX}-priv.pem"
PQC_CREATE_BOOT_BLOBS_ARGS+=" --infile-signkey-${PREFIX_ARG}-root-pqc=${BASEDIR_RSAKEY_ROOT}/key/${PREFIX}root${ml_dsa_algo_name}-${ROOTRSA_INDEX}-priv.pem"

### Input: Level-1 Cert ###
RSA_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-lvl1cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${rsa_algo_name}-pub.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-pubkey-${PREFIX_ARG}-lvl1cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${rsa_algo_name}-pub.pem"
PQC_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-lvl1cert-pqc=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${ml_dsa_algo_name}-pub.pem"

COMMON_CREATE_BOOT_BLOBS_ARGS+=" --infile-epks-${PREFIX_ARG}-lvl1cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl1cert-epks.bin"
COMMON_CREATE_BOOT_BLOBS_ARGS+=" --infile-nonce-${PREFIX_ARG}-lvl1rsa=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/nonce/${PREFIX}lvl1key-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-epks-${PREFIX_ARG}-lvl1cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl1cert-epks.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-${PREFIX_ARG}-lvl1rsa=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/nonce/${PREFIX}lvl1key-nonce.bin"

RSA_CREATE_BOOT_BLOBS_ARGS+=" --infile-signkey-${PREFIX_ARG}-lvl1=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${rsa_algo_name}-priv.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-signkey-${PREFIX_ARG}-lvl1=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${rsa_algo_name}-priv.pem"
PQC_CREATE_BOOT_BLOBS_ARGS+=" --infile-signkey-${PREFIX_ARG}-lvl1-pqc=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-1-${ml_dsa_algo_name}-priv.pem"

### Input: Level-2 Cert ###
RSA_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-lvl2cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${rsa_algo_name}-pub.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-pubkey-${PREFIX_ARG}-lvl2cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${rsa_algo_name}-pub.pem"
PQC_CREATE_BOOT_BLOBS_ARGS+=" --infile-pubkey-${PREFIX_ARG}-lvl2cert-pqc=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${ml_dsa_algo_name}-pub.pem"

COMMON_CREATE_BOOT_BLOBS_ARGS+=" --infile-epks-${PREFIX_ARG}-lvl2cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl2cert-epks.bin"
COMMON_CREATE_BOOT_BLOBS_ARGS+=" --infile-nonce-${PREFIX_ARG}-lvl2rsa=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/nonce/${PREFIX}lvl2key-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-epks-${PREFIX_ARG}-lvl2cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl2cert-epks.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-${PREFIX_ARG}-lvl2rsa=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/nonce/${PREFIX}lvl2key-nonce.bin"

RSA_CREATE_DEVICE_FIP_ARGS+=" --infile-signkey-${PREFIX_ARG}-lvl2=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${rsa_algo_name}-priv.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-signkey-${PREFIX_ARG}-lvl2=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${rsa_algo_name}-priv.pem"
PQC_CREATE_DEVICE_FIP_ARGS+=" --infile-signkey-${PREFIX_ARG}-lvl2-pqc=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/key/${PREFIX}level-2-${ml_dsa_algo_name}-priv.pem"

### Input: Chipset Level-3 Certs  ###
RSA_CREATE_DEVICE_FIP_ARGS+=" --infile-pubkey-bl40-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/key/${PREFIX}bl40-level-3-${rsa_algo_name}-pub.pem"
RSA_CREATE_DEVICE_FIP_ARGS+=" --infile-epks-bl40-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/epk/${PREFIX}bl40-lvl3cert-epks.bin"
RSA_CREATE_DEVICE_FIP_CS_ARGS+=" --infile-nonce-bl40-cslvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/${PREFIX}bl40-lvl3key-nonce.bin"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-nonce-bl40-dvlvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/${PREFIX}bl40-lvl3key-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-pubkey-bl40-chipset-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/key/cs-bl40-level-3-${rsa_algo_name}-pub.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-epks-bl40-chipset-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/epk/cs-bl40-lvl3cert-epks.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-bl40-cslvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/cs-bl40-lvl3key-nonce.bin"

RSA_CREATE_DEVICE_FIP_ARGS+=" --infile-pubkey-bl31-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/key/${PREFIX}bl31-level-3-${rsa_algo_name}-pub.pem"
RSA_CREATE_DEVICE_FIP_ARGS+=" --infile-epks-bl31-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/epk/${PREFIX}bl31-lvl3cert-epks.bin"
RSA_CREATE_DEVICE_FIP_CS_ARGS+=" --infile-nonce-bl31-cslvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/${PREFIX}bl31-lvl3key-nonce.bin"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-nonce-bl31-dvlvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/${PREFIX}bl31-lvl3key-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-pubkey-bl31-chipset-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/key/cs-bl31-level-3-${rsa_algo_name}-pub.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-epks-bl31-chipset-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/epk/cs-bl31-lvl3cert-epks.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-bl31-cslvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/cs-bl31-lvl3key-nonce.bin"

RSA_CREATE_DEVICE_FIP_ARGS+=" --infile-pubkey-bl32-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/key/${PREFIX}bl32-level-3-${rsa_algo_name}-pub.pem"
RSA_CREATE_DEVICE_FIP_ARGS+=" --infile-epks-bl32-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/epk/${PREFIX}bl32-lvl3cert-epks.bin"
RSA_CREATE_DEVICE_FIP_CS_ARGS+=" --infile-nonce-bl32-cslvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/${PREFIX}bl32-lvl3key-nonce.bin"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-nonce-bl32-dvlvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/${PREFIX}bl32-lvl3key-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-pubkey-bl32-chipset-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/key/cs-bl32-level-3-${rsa_algo_name}-pub.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-epks-bl32-chipset-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/epk/cs-bl32-lvl3cert-epks.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-bl32-cslvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/cs-bl32-lvl3key-nonce.bin"

### Input: Chipset Level-3 privae RSA Keys ###
#BB1ST_ARGS="${BB1ST_ARGS} --infile-signkey-bl40-chipset-lvl3=${BASEDIR_FIP_RSAKEY_ROOT}/key/cs-bl40-level-3-rsa-priv.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-signkey-bl31-chipset-lvl3=${BASEDIR_FIP_RSAKEY_ROOT}/key/cs-bl31-level-3-rsa-priv.pem"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-signkey-bl32-chipset-lvl3=${BASEDIR_FIP_RSAKEY_ROOT}/key/cs-bl32-level-3-rsa-priv.pem"

RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-pubkey-bl30-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/key/bl30-level-3-${rsa_algo_name}-pub.pem"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-epks-bl30-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/epk/bl30-lvl3cert-epks.bin"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-nonce-bl30-dvlvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/bl30-lvl3key-nonce.bin"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-pubkey-bl33-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/key/bl33-level-3-${rsa_algo_name}-pub.pem"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-epks-bl33-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/epk/bl33-lvl3cert-epks.bin"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-nonce-bl33-dvlvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/bl33-lvl3key-nonce.bin"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-pubkey-krnl-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/key/krnl-level-3-${rsa_algo_name}-pub.pem"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-epks-krnl-${PREFIX_ARG}-lvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/epk/krnl-lvl3cert-epks.bin"
RSA_CREATE_DEVICE_FIP_DV_ARGS+=" --infile-nonce-krnl-dvlvl3cert=${BASEDIR_FIP_RSAKEY_ROOT}/nonce/krnl-lvl3key-nonce.bin"

if [ ${with_encryption} -eq 1 ]; then
	### Input: Protection RootKey ###
	COMMON_CREATE_BOOT_BLOBS_ARGS+=" --infile-aes256-${PREFIX_ARG}-rootkey-2=${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX_BS}rootkey-bootstage-2.bin"
	COMMON_CREATE_BOOT_BLOBS_ARGS+=" --infile-aes256-${PREFIX_ARG}-rootkey-3=${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX_BS}rootkey-bootstage-3.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --infile-aes256-${PREFIX_ARG}-rootkey-2=${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX}rootkey-bootstage-2.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --infile-aes256-${PREFIX_ARG}-rootkey-3=${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX}rootkey-bootstage-3.bin"

	COMMON_CREATE_DEVICE_FIP_ARGS+=" --infile-aes256-${PREFIX_ARG}-rootkey-1=${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX_BS}rootkey-bootstage-1.bin"
	COMMON_CREATE_DEVICE_FIP_ARGS+=" --infile-epks-${PREFIX_ARG}-lvl2cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl2cert-epks.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --infile-aes256-${PREFIX_ARG}-rootkey-1=${BASEDIR_AESKEY_ROOT}/aes256-${PREFIX}rootkey-bootstage-1.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --infile-epks-${PREFIX_ARG}-lvl2cert=${BASEDIR_BOOTBLOBS_RSAKEY_ROOT}/epk/${PREFIX}lvl2cert-epks.bin"
fi

### Input: nonce for binary protection ###
### If not supplied, will be randomly generated by acpu-imagetool ###
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-csinit-params=${BASEDIR_NONCE}/chipset/blob/csinit-params-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-ddr-fwdata=${BASEDIR_NONCE}/chipset/blob/ddr-fwdata-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-blob-bl2=${BASEDIR_NONCE}/chipset/blob/blob-bl2-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-blob-bl2e=${BASEDIR_NONCE}/chipset/blob/blob-bl2e-nonce.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --infile-nonce-blob-bl2x=${BASEDIR_NONCE}/chipset/blob/blob-bl2x-nonce.bin"

### Features, flags and switches ###
if [ ${with_encryption} -eq 1 ]; then
	COMMON_CREATE_BOOT_BLOBS_CS_ARGS+=" --feature-enable-chipset-pubrsa-prot"
	#BB1ST_ARGS="${BB1ST_ARGS} --feature-enable-${PREFIX_ARG}-pubrsa-prot"

	COMMON_CREATE_DEVICE_FIP_CS_ARGS+=" --feature-enable-chipset-pubrsa-prot"

	COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --feature-enable-device-root-pubrsa-prot"
	COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --feature-enable-device-lvl1-pubrsa-prot"
	COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --feature-enable-device-lvlx-pubrsa-prot"
	COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --feature-device-root-pubrsa-prot-mrk"
	#BB1ST_ARGS="${BB1ST_ARGS} --feature-enable-${PREFIX_ARG}-lvl1-pubrsa-prot"
	#BB1ST_ARGS="${BB1ST_ARGS} --feature-enable-${PREFIX_ARG}-lvlx-pubrsa-prot"
	#BB1ST_ARGS="${BB1ST_ARGS} --feature-${PREFIX_ARG}-root-pubrsa-prot-mrk"

	COMMON_CREATE_DEVICE_FIP_DV_ARGS+=" --feature-enable-device-lvlx-pubrsa-prot"
fi

# arb info
COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --val-device-scs-segid=${DEVICE_SCS_SEGID}"
COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --val-device-vendor-segid=${DEVICE_VENDOR_SEGID}"
COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --val-device-scs-vers=${DEVICE_SCS_VERS}"
COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --val-device-tee-vers=${DEVICE_TEE_VERS}"
if  [ "1" == "${DEVICE_SCS_LVL1CERT_VERS_SUBMASK}" ]; then
COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --switch-device-lvl1cert-vers-submask=1"
fi

COMMON_CREATE_DEVICE_FIP_DV_ARGS+=" --val-device-vendor-segid=${DEVICE_VENDOR_SEGID}"
COMMON_CREATE_DEVICE_FIP_DV_ARGS+=" --val-device-tee-vers=${DEVICE_TEE_VERS}"
COMMON_CREATE_DEVICE_FIP_DV_ARGS+=" --val-device-ree-vers=${DEVICE_REE_VERS}"

### Output: blobs ###
if [ ${write_bb1st_template} -eq 1 ]; then
	COMMON_CREATE_BOOT_BLOBS_ARGS+=" --outfile-bb1st=${BASEDIR_OUTPUT_BLOB}/bb1st.bin${output_file_ext}"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-bb1st=${BASEDIR_OUTPUT_BLOB}/bb1st.bin${output_file_ext}"

	#COMMON_CREATE_DEVICE_FIP_ARGS+=" --outfile-device-fip=${BASEDIR_FIP_OUTPUT_BLOB}/device-fip.bin${output_file_ext}"
	COMMON_CREATE_DEVICE_FIP_ARGS+=" --outfile-device-fip-header=${BASEDIR_FIP_OUTPUT_BLOB}/device-fip-header.bin${output_file_ext}"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-device-fip=${BASEDIR_FIP_OUTPUT_BLOB}/device-fip.bin${output_file_ext}"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-device-fip-header=${BASEDIR_FIP_OUTPUT_BLOB}/device-fip-header.bin${output_file_ext}"
fi

#BB1ST_ARGS="${BB1ST_ARGS} --outfile-bb1st=${BASEDIR_OUTPUT_BLOB}/blob-bl2e.bin"
#BB1ST_ARGS="${BB1ST_ARGS} --outfile-bb1st=${BASEDIR_OUTPUT_BLOB}/blob-bl2x.bin"

### Output: hash of root cert ###
if [ ${write_root_hash} -eq 1 ]; then
	COMMON_CREATE_BOOT_BLOBS_ARGS+=" --outfile-hash-${PREFIX_ARG}-rootcert=${BASEDIR_OUTPUT_BLOB}/hash-${PREFIX_BS}rootcert-${sig_scheme}.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-hash-${PREFIX_ARG}-rootcert=${BASEDIR_OUTPUT_BLOB}/hash-${PREFIX}rootcert-${sig_scheme}.bin"
fi

### Output: generated protection keys ###
if [ ${with_encryption} -eq 1 ]; then
	COMMON_CREATE_BOOT_BLOBS_CS_ARGS+=" --outfile-protkey-bl2=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-bl2.bin"
	COMMON_CREATE_BOOT_BLOBS_CS_ARGS+=" --outfile-protkey-bl2e=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-bl2e.bin"
	COMMON_CREATE_BOOT_BLOBS_CS_ARGS+=" --outfile-protkey-bl2x=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-bl2x.bin"
	COMMON_CREATE_BOOT_BLOBS_CS_ARGS+=" --outfile-protkey-csinit-params=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-csinit-params.bin"
	COMMON_CREATE_BOOT_BLOBS_CS_ARGS+=" --outfile-protkey-ddr-fwdata=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddr-fwdata.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-bl2=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-bl2.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-bl2e=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-bl2e.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-bl2x=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-bl2x.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-csinit-params=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-csinit-params.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-ddr-fwdata=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddr-fwdata.bin"

	if [ "${scs_family}" == "" ] || [ "${scs_family}" == "sc2" ]; then
		COMMON_CREATE_BOOT_BLOBS_ARGS+=" --outfile-protkey-ddrfw-ddr3-1d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-ddr3-1d.bin"
		COMMON_CREATE_BOOT_BLOBS_ARGS+=" --outfile-protkey-ddrfw-ddr4-1d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-ddr4-1d.bin"
		COMMON_CREATE_BOOT_BLOBS_ARGS+=" --outfile-protkey-ddrfw-ddr4-1d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-ddr4-2d.bin"
		COMMON_CREATE_BOOT_BLOBS_ARGS+=" --outfile-protkey-ddrfw-lpddr3-1d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-lpddr3-1d.bin"
		COMMON_CREATE_BOOT_BLOBS_ARGS+=" --outfile-protkey-ddrfw-lpddr4-1d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-lpddr4-1d.bin"
		COMMON_CREATE_BOOT_BLOBS_ARGS+=" --outfile-protkey-ddrfw-lpddr4-2d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-lpddr4-2d.bin"
		#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-ddrfw-ddr3-1d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-ddr3-1d.bin"
		#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-ddrfw-ddr4-1d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-ddr4-1d.bin"
		#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-ddrfw-ddr4-2d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-ddr4-2d.bin"
		#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-ddrfw-lpddr3-1d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-lpddr3-1d.bin"
		#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-ddrfw-lpddr4-1d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-lpddr4-1d.bin"
		#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-ddrfw-lpddr4-2d=${BASEDIR_OUTPUT_PROTKEY}/genkey-prot-ddrfw-lpddr4-2d.bin"
	fi

	COMMON_CREATE_DEVICE_FIP_CS_ARGS+=" --outfile-protkey-bl40=${BASEDIR_FIP_OUTPUT_PROTKEY}/genkey-prot-bl40.bin"
	COMMON_CREATE_DEVICE_FIP_CS_ARGS+=" --outfile-protkey-bl31=${BASEDIR_FIP_OUTPUT_PROTKEY}/genkey-prot-bl31.bin"
	COMMON_CREATE_DEVICE_FIP_CS_ARGS+=" --outfile-protkey-bl32=${BASEDIR_FIP_OUTPUT_PROTKEY}/genkey-prot-bl32.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-bl40=${BASEDIR_FIP_OUTPUT_PROTKEY}/genkey-prot-bl40.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-bl31=${BASEDIR_FIP_OUTPUT_PROTKEY}/genkey-prot-bl31.bin"
	#BB1ST_ARGS="${BB1ST_ARGS} --outfile-protkey-bl32=${BASEDIR_FIP_OUTPUT_PROTKEY}/genkey-prot-bl32.bin"

	COMMON_CREATE_DEVICE_FIP_DV_ARGS+=" --outfile-protkey-bl30=${BASEDIR_FIP_OUTPUT_PROTKEY}/genkey-prot-bl30.bin"
	COMMON_CREATE_DEVICE_FIP_DV_ARGS+=" --outfile-protkey-bl33=${BASEDIR_FIP_OUTPUT_PROTKEY}/genkey-prot-bl33.bin"
	COMMON_CREATE_DEVICE_FIP_DV_ARGS+=" --outfile-protkey-krnl=${BASEDIR_FIP_OUTPUT_PROTKEY}/genkey-prot-krnl.bin"
fi

if [ ${OPS} == "create-boot-blobs" ]; then
	if [ ${trust_chain^^} == "CHIPSET" ]; then
		COMMON_CREATE_BOOT_BLOBS_ARGS+=$COMMON_CREATE_BOOT_BLOBS_CS_ARGS
		RSA_CREATE_BOOT_BLOBS_ARGS+=$RSA_CREATE_BOOT_BLOBS_CS_ARGS
		PQC_CREATE_BOOT_BLOBS_ARGS+=$PQC_CREATE_BOOT_BLOBS_CS_ARGS
	else
		COMMON_CREATE_BOOT_BLOBS_DV_ARGS+=" --switch-device-sign-blob=0"
		COMMON_CREATE_BOOT_BLOBS_ARGS+=$COMMON_CREATE_BOOT_BLOBS_DV_ARGS
		RSA_CREATE_BOOT_BLOBS_ARGS+=$RSA_CREATE_BOOT_BLOBS_DV_ARGS
		PQC_CREATE_BOOT_BLOBS_ARGS+=$PQC_CREATE_BOOT_BLOBS_DV_ARGS
	fi

	trace "Check Common File List"
	for f in $COMMON_CREATE_BOOT_BLOBS_FILE_CHECKLIST; do
		trace "    $f"
		check_file $f
	done

	trace "Check RSA File List"
	if [ ${is_rsa} -eq 1 ]; then
		for f in $RSA_CREATE_BOOT_BLOBS_FILE_CHECKLIST; do
			trace "    $f"
			check_file $f
		done
	fi

	trace "Check PQC File List"
	if [ ${is_ml_dsa} -eq 1 ]; then
		for f in $PQC_CREATE_BOOT_BLOBS_FILE_CHECKLIST; do
			trace "    $f"
			check_file $f
		done
	fi

	trace "Common CREATE_BOOT_BLOBS Args"
	for i in $COMMON_CREATE_BOOT_BLOBS_ARGS; do
		trace "    $i"
	done
	BB1ST_ARGS="${BB1ST_ARGS} $COMMON_CREATE_BOOT_BLOBS_ARGS"

	trace "RSA CREATE_BOOT_BLOBS Args"
	for i in $RSA_CREATE_BOOT_BLOBS_ARGS; do
		trace "    $i"
	done

	if [ ${is_rsa} -eq 1 ]; then
		BB1ST_ARGS="${BB1ST_ARGS} $RSA_CREATE_BOOT_BLOBS_ARGS"
	fi

	trace "PQC CREATE_BOOT_BLOBS Args"
	for i in $PQC_CREATE_BOOT_BLOBS_ARGS; do
		trace "    $i"
	done
	if [ ${is_ml_dsa} -eq 1 ]; then
		BB1ST_ARGS="${BB1ST_ARGS} $PQC_CREATE_BOOT_BLOBS_ARGS"
	fi
elif [ ${OPS} == "create-device-fip" ]; then
	if [ ${trust_chain^^} == "CHIPSET" ]; then
		COMMON_CREATE_DEVICE_FIP_ARGS+=$COMMON_CREATE_DEVICE_FIP_CS_ARGS
		RSA_CREATE_DEVICE_FIP_ARGS+=$RSA_CREATE_DEVICE_FIP_CS_ARGS
		PQC_CREATE_DEVICE_FIP_ARGS+=$PQC_CREATE_DEVICE_FIP_CS_ARGS
	else
		COMMON_CREATE_DEVICE_FIP_ARGS+=$COMMON_CREATE_DEVICE_FIP_DV_ARGS
		RSA_CREATE_DEVICE_FIP_ARGS+=$RSA_CREATE_DEVICE_FIP_DV_ARGS
		PQC_CREATE_DEVICE_FIP_ARGS+=$PQC_CREATE_DEVICE_FIP_DV_ARGS
	fi

	trace "Check Common File List"
	for f in $COMMON_CREATE_DEVICE_FIP_FILE_CHECKLIST; do
		trace "    $f"
		check_file $f
	done

	trace "Check RSA File List"
	if [ ${is_rsa} -eq 1 ]; then
		for f in $RSA_CREATE_DEVICE_FIP_FILE_CHECKLIST; do
			trace "    $f"
			check_file $f
		done
	fi

	trace "Check PQC File List"
	if [ ${is_ml_dsa} -eq 1 ]; then
		for f in $PQC_CREATE_DEVICE_FIP_FILE_CHECKLIST; do
			trace "    $f"
			check_file $f
		done
	fi

	trace "Common CREATE_DEVICE_FIP Args"
	for i in $COMMON_CREATE_DEVICE_FIP_ARGS; do
		trace "    $i"
	done
	BB1ST_ARGS="${BB1ST_ARGS} $COMMON_CREATE_DEVICE_FIP_ARGS"

	trace "RSA CREATE_DEVICE_FIP Args"
	for i in $RSA_CREATE_DEVICE_FIP_ARGS; do
		trace "    $i"
	done

	if [ ${is_rsa} -eq 1 ]; then
		BB1ST_ARGS="${BB1ST_ARGS} $RSA_CREATE_DEVICE_FIP_ARGS"
	fi

	trace "PQC CREATE_DEVICE_FIP Args"
	for i in $PQC_CREATE_DEVICE_FIP_ARGS; do
		trace "    $i"
	done
	if [ ${is_ml_dsa} -eq 1 ]; then
		BB1ST_ARGS="${BB1ST_ARGS} $PQC_CREATE_DEVICE_FIP_ARGS"
	fi
fi

trace "BB1ST_ARGS Dump"
trace "    $OPS"
for i in $BB1ST_ARGS; do
	trace "    $i"
done

#
# Main
#

set -x
${ACPU_IMAGETOOL} \
	${OPS} \
	${BB1ST_ARGS}

if [ ${write_root_hash} -eq 1 ]; then
    sha256sum ${BASEDIR_OUTPUT_BLOB}/hash-${PREFIX_BS}rootcert-${sig_scheme}.bin
    cp ${BASEDIR_OUTPUT_BLOB}/hash-${PREFIX_BS}rootcert-${sig_scheme}.bin ${BASEDIR_OUTPUT_HASH}/hash-${PREFIX_BS}rootcert-${sig_scheme}.bin
fi
# vim: set tabstop=4 noexpandtab shiftwidth=4:
