#!/bin/bash -e

# Copyright (c) 2020 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.

#set -x
version=1.0

EXEC_BASEDIR=$(dirname $(readlink -f $0))
BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

trace ()
{
	echo ">>> $@" > /dev/null
	#echo ">>> $@"
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

usage() {
	cat << EOF
Usage: $(basename $0) --help | --version

		Generate all Amlogic Device Vendor Secure Chipset Startup (SCS) keys

		$(basename $0)
		--key-dir <key-dir> \\
		{--rsa-size [2048 | 4096]} \\
		{--sig-scheme [rsa-only | rsa-mldsa-hybrid | mldsa-only]} \\
		{--ml-dsa-level [2 | 3 | 5]} \\
		{--project <project-name>} \\
		--template-dir  <template-dir> \\
		--rootkey-index [0 | 1 | 2 | 3] \\
		--out-dir <output-dir>

EOF
    exit 1
}

key_dir=""
part=""
size=""
template_dir=""
rootkey_index=0
output_dir=""
device_soc="s6"
scs_family="s7d"
ml_dsa_level="3"
sig_scheme="rsa-mldsa"
sig_scheme_full="rsa-mldsa-draft1"
template_layout="rsa-mldsa"
prefix="none"
create_boot_blobs_args=""

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
			--key-dir)
				key_dir="${argv[$i]}"
				;;
			--project)
				part="${argv[$i]}"
				;;
			--device-soc)
				device_soc="${argv[$i]}"
				;;
			--rsa-size)
				size="${argv[$i]}"
				;;
			--sig-scheme)
				sig_scheme="${argv[$i]}"
				;;
			--prefix)
				prefix="${argv[$i]}"
				;;
			--ml-dsa-level)
				ml_dsa_level="${argv[$i]}"
				;;
			--template-dir)
				template_dir="${argv[$i]}"
				check_dir "${template_dir}"
				;;
			--template-layout)
				template_layout="${argv[$i]}"
				;;
			--rootkey-index)
				rootkey_index="${argv[$i]}"
				check_value "$rootkey_index" 0 3
				;;
			--device-lvl1cert-vers-submask)
				create_boot_blobs_args+="--device-lvl1cert-vers-submask ${argv[$i]}"
				;;
			--out-dir)
				output_dir="${argv[$i]}"
				;;
			*)
				echo "Unknown option $arg";
				usage
				;;
		esac
		i=$((i + 1))
	done
}

parse_main "$@"

trace "  key-dir $key_dir"
trace "  project $part"
trace "  rsa-size $size"
trace "  sig-scheme $sig_scheme"
trace "  ml-dsa-level $ml_dsa_level"
trace "  prefix $prefix"
trace "  template_dir $template_dir"
trace "  rootkey-index $rootkey_index"
trace "  out-dir $output_dir"

if [ -z "$key_dir" ]; then
	usage
fi

if [ -z "$size" ]; then
	size=4096
fi

if [ -z "$template_dir" ]; then
	usage
fi

if [ -z "$rootkey_index" ]; then
	usage
fi

if [ -z "$output_dir" ]; then
	usage
fi

if [ "$size" -ne 2048 ] && [ "$size" -ne 4096 ]; then
	echo "Error: Invalid RSA key size $size"
	usage
fi

case "$sig_scheme" in
	rsa|rsa-only)
		sig_scheme=rsa
		sig_scheme_full=rsa
		;;
	rsa-mldsa|rsa-mldsa-hybrid)
		sig_scheme=rsa-mldsa
		sig_scheme_full=rsa-mldsa-draft1
		;;
	mldsa|mldsa-only)
		sig_scheme=mldsa
		sig_scheme_full=mldsa-draft1
		;;
	*) usage ;;
esac

case "$ml_dsa_level" in
	2|3|5) ;;
	*) usage ;;
esac

mkdir -p "$key_dir"
${EXEC_BASEDIR}/bin/gen_scs_keys.sh --key-dir "$key_dir" --sig-scheme $sig_scheme --prefix $prefix --stage root --rsa-size "$size" --ml-dsa-level $ml_dsa_level --ml-dsa-version draft1 --project "$part" --link-lvl3-to-lvl2-file 1
${EXEC_BASEDIR}/bin/gen_scs_keys.sh --key-dir "$key_dir" --sig-scheme $sig_scheme --prefix $prefix --stage boot-blobs --rsa-size "$size" --ml-dsa-level $ml_dsa_level --ml-dsa-version draft1 --project "$part" --link-lvl3-to-lvl2-file 1
${EXEC_BASEDIR}/bin/gen_scs_keys.sh --key-dir "$key_dir" --sig-scheme $sig_scheme --prefix $prefix --stage fip --rsa-size "$size" --ml-dsa-level $ml_dsa_level --ml-dsa-version draft1 --project "$part" --link-lvl3-to-lvl2-file 1

mkdir -p "$key_dir"/root/dvgk/"$part"
${EXEC_BASEDIR}/bin/dvgk_gen.sh "$key_dir"/root/dvgk/"$part"/dvgk

mkdir -p "$key_dir"/root/dvuk/"$part"
${EXEC_BASEDIR}/bin/dvuk_gen.sh "$key_dir"/root/dvuk/"$part"/dvuk

${EXEC_BASEDIR}/bin/derive_device_aes_rootkey.sh --key-dir "$key_dir" --mrk-bin "$key_dir"/root/dvgk/"$part"/dvgk.bin --mrk-name DVGK --project "$part"


${EXEC_BASEDIR}/bin/gen_scs_root_hash.sh --rootkey-index $rootkey_index --key-dir "$key_dir" --trust-chain device-vendor --project "$part" --device-soc "$device_soc" --template-dir "$template_dir" --sig-scheme $sig_scheme --ml-dsa-version draft1 --scs-family $scs_family --template-layout $template_layout --fip-header-layout mini --device-lvl1cert-vers-submask 1 --ops create-boot-blobs
${EXEC_BASEDIR}/bin/gen_scs_root_hash.sh --rootkey-index $rootkey_index --key-dir "$key_dir" --trust-chain device-vendor --project "$part" --device-soc "$device_soc" --template-dir "$template_dir" --sig-scheme $sig_scheme --ml-dsa-version draft1 --scs-family $scs_family --template-layout $template_layout --fip-header-layout mini --ops create-device-fip

# Link to be compatible with old script
cp "$key_dir/fip/aes/${part}/trustchain-${rootkey_index}/protkey" \
	"$key_dir/fip/aes/${part}/protkey" -r

${EXEC_BASEDIR}/bin/export_dv_scs_signing_keys.sh --key-dir "$key_dir" --out-dir "$output_dir" --rootkey-index "$rootkey_index" --project "$part" --sig-scheme $sig_scheme  --ml-dsa-version draft1 --template-layout $template_layout
