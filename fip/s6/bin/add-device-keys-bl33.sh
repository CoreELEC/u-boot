#!/bin/bash -e

# Copyright (c) 2020 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.

#set -x

SCRIPT_PATH=${SCRIPT_PATH:-$(dirname $(readlink -f $0))}

# Temporary files directory
if [ "$TMP" == "/tmp" ] || [ -z "$TMP" ]; then
    TMP=${SCRIPT_PATH}/tmp
fi

trace ()
{
    echo ">>> $@" > /dev/null
}

usage() {
    cat << EOF
Usage: $(basename $0) --help

       Amlogic Device Vendor Secure Chipset Startup (SCS) Signing

       $(basename $0) --add-device-keys	\\
		--key-dir <key-dir> \\
		--project <project-name> \\
		{--sig-scheme [rsa-only | rsa-mldsa-hybrid | mldsa-only]} \\
		--input  <input img> \\
		{--rootkey-index [0 | 1 | 2 | 3]} \\
		--output <output img>
EOF
    exit 1
}

check_file() {
    if [ ! -f "$2" ]; then echo Error: Unable to open $1: \""$2"\"; exit 1 ; fi
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

# struct scs_cmn_hdr_t {
#	 u64 magic_u64;
#	 u16 version_u16;
#	 u16 flags_u16;
#	 u32 length_u32;
# };
# u64 magic = 0x38a41024204c4d40;

append_uint32_le() {
    local input=$1
    local output=$2
    local v=
    local vrev=
    v=$(printf %08x $input)
    # 00010001
    vrev=${v:6:2}${v:4:2}${v:2:2}${v:0:2}

    echo $vrev | xxd -r -p >> $output
}

# Convert RSA PEM key to precomputed binary key file
# If input is already the precomputed binary key file, then it is simply copied
# to the output
# $1: input RSA private .PEM
# $2: output precomputed binary key file
# $3: enable N0INV128 if True
pem_to_bin() {
    local input=$1
    local output=$2
    local n0inv128_enable=$3
    if [ ! -f "$1" ] || [ -z "$2" ]; then
        echo "Argument error, \"$1\", \"$2\" "
        exit 1
    fi
    if [ -z "$n0inv128_enable" ]; then
      n0inv128_enable=False
    fi

    local insize=$(wc -c < $input)
    if [ $insize -eq 1052 ] || [ $insize -eq 1036 ]; then
        # input is already precomputed binary key file
        cp $input $output
    fi

    local pycmd="import sys; \
                 sys.path.append(\"${SCRIPT_PATH}\"); \
                 import pem_extract_pubkey; \
                 sys.stdout.write(pem_extract_pubkey.extract_pubkey( \
                    \"$input\", headerMode=False, n0inv128Enable=$n0inv128_enable));"
    /usr/bin/env python -c "$pycmd" > $output
}

add_device_keys() {
    local input=""
    local key_dir=""
    local part=""
    local rootkey_index=0
    local output=""
    local bl33_rsakey=""
    local krnl_rsakey=""
    local sig_scheme="rsa-mldsa"
    local argv=("$@")
    local i=0

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --key-dir)
                key_dir="${argv[$i]}"
        check_dir "${key_dir}"
        ;;
            --project)
                part="${argv[$i]}"
        ;;
            --sig-scheme)
                sig_scheme="${argv[$i]}"
        ;;
            --input)
                input="${argv[$i]}"
        ;;
            --rootkey-index)
                rootkey_index="${argv[$i]}"
        check_value "${rootkey_index}" 0 3
        ;;
            --output)
                output="${argv[$i]}"
        ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done

    # Verify args
    if [ -z "${key_dir}" ]; then
        usage
    fi

    if [ -z "${part}" ]; then
        echo "Error: project cannot be empty"
        usage
    fi

    if [ -z "${rootkey_index}" ]; then
        rootkey_index=0
    fi

    check_file "input" "${input}"
    if [ -z "${output}" ]; then
        echo Error: Missing output file option --output; exit 1;
    fi

case "$sig_scheme" in
	rsa|rsa-only)
		sig_scheme=rsa
		;;
	rsa-mldsa|rsa-mldsa-hybrid)
		sig_scheme=rsa-mldsa
		;;
	mldsa|mldsa-only)
		sig_scheme=mldsa
		;;
	*) usage ;;
esac

    bl33_rsakey=$(readlink -f ${key_dir})/fip/${sig_scheme}/${part}/trustchain-${rootkey_index}/key/bl33-level-3-rsa-priv.pem
    check_file "BL33 RSA key" "${bl33_rsakey}"
    krnl_rsakey=$(readlink -f ${key_dir})/fip/${sig_scheme}/${part}/trustchain-${rootkey_index}/key/krnl-level-3-rsa-priv.pem
    check_file "Kernel RSA key" "${krnl_rsakey}"

    pem_to_bin $bl33_rsakey $TMP/bl33key.bin True
    pem_to_bin $krnl_rsakey $TMP/krnlkey.bin True

    # Create header
    # magic_u64, version_u16, flags_u16, length_u32
    append_uint32_le 0x204c4d40 $TMP/key.hdr
    append_uint32_le 0x38a41024 $TMP/key.hdr
    append_uint32_le 0 $TMP/key.hdr
    append_uint32_le 1072 $TMP/key.hdr

    cat $TMP/key.hdr $TMP/bl33key.bin > $TMP/bl33key-pl.bin
    cat $TMP/key.hdr $TMP/krnlkey.bin > $TMP/krnlkey-pl.bin

    if [ ${input} != ${output} ]; then
        cp ${input} ${output}
    fi
    dd if=$TMP/bl33key-pl.bin of=${output} bs=1 seek=4 conv=notrunc &> /dev/null
    dd if=$TMP/krnlkey-pl.bin of=${output} bs=1 seek=2052 conv=notrunc &> /dev/null
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
        case "$arg" in
            -h|--help)
                usage
                break ;;
            --add-device-keys)
                add_device_keys "${argv[@]:$((i + 1))}"
                break ;;
            *)
                echo "Unknown first option $1"; exit 1
                ;;
        esac
        i=$((i + 1))
    done
}

cleanup() {
    if [ ! -d "$TMP" ]; then return; fi
    local tmpfiles="$TMP/bl33key.bin $TMP/krnlkey.bin $TMP/key.hdr $TMP/bl33key-pl.bin $TMP/krnlkey-pl.bin "
    for i in $tmpfiles ; do
        rm -f $TMP/$i
    done
    rm -fr $TMP
}

trap cleanup EXIT

cleanup
if [ ! -d "$TMP" ]; then mkdir "$TMP" ; fi
parse_main "$@"
