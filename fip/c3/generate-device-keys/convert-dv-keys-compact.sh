#!/bin/bash

#set -x
set -o pipefail
set -o errexit
set -o errtrace
trap "{ echo Error: Line $LINENO \"$BASH_COMMAND\" returned $? ; exit 1; }" ERR

rsa_copy() {
	local chain_num=$1
	local path=$2
	local files=$3
	local src_key=$4

	echo "Copy $chain_num RSA key ..."

	for f in $files
	do
		local kpriv="$path/$f-priv.pem"
		local kpub="$path/$f-pub.pem"
		local src_kpriv="$path/$src_key-priv.pem"
		local src_kpub="$path/$src_key-pub.pem"
		cp $src_kpriv $kpriv
		cp $src_kpub $kpub
		echo $kpriv
		echo $kpub
	done
}

ek_copy() {
	local chain_num=$1
	local path=$2
	local files=$3
	local src_file=$4

	echo "Copy $chain_num EKs ..."

	for f in $files
	do
		local file="$path/$f"
		cp "$path/$src_file" $file
		echo $file
	done
}

nonce_copy() {
	local chain_num=$1
	local path=$2
	local files=$3
	local src_file=$4

	echo "Copy $chain_num NONCE ..."

	for f in $files
	do
		local file="$path/$f"
		cp "$path/$src_file" $file
		echo $file
	done
}

kd="${1:-}"
if [ -z "$kd" ] || [ ! -d "$kd" ]; then
    echo "Usage: $0 dv_scs_keys"
    echo "Convert dv_scs_keys key directory to compact version."
    exit 1
fi

if [ ! -d "$kd/boot-blobs" ] &&
        [ ! -d "$kd/fip" ] &&
        [ ! -d "$kd/root" ]; then
    echo "Error: Unable to find boot-blobs, fip or root directories"
    exit 1
fi

if [ -d "$kd/fip/rsa" ]; then
    for part in "$kd"/fip/rsa/*; do
        part="${part%/}"
        for i in 0 1 2 3
        do
            fip_rsa_path=$part/rootrsa-${i}

            rsa_copy $i "$fip_rsa_path/key" "bl31-level-3-rsa bl32-level-3-rsa bl33-level-3-rsa bl40-level-3-rsa" "bl30-level-3-rsa"
            ek_copy $i "$fip_rsa_path/epk" "bl31-lvl3cert-epks.bin bl32-lvl3cert-epks.bin bl33-lvl3cert-epks.bin bl40-lvl3cert-epks.bin" "bl30-lvl3cert-epks.bin"
            nonce_copy $i "$fip_rsa_path/nonce" "bl31-dvlvl3cert-nonce.bin bl32-dvlvl3cert-nonce.bin bl33-dvlvl3cert-nonce.bin bl40-dvlvl3cert-nonce.bin" "bl30-dvlvl3cert-nonce.bin"
        done
    done
fi




