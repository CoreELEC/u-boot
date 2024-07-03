#!/bin/bash

#set -x
set -o pipefail
set -o errexit
set -o errtrace
trap "{ echo Error: Line $LINENO \"$BASH_COMMAND\" returned $? ; exit 1; }" ERR

mv_if_exists() {
    # 1: src, 2: dst
    if [ -e "$1" ]; then
        mv -v "$1" "$2"
    fi
}

kd="${1:-}"
if [ -z "$kd" ] || [ ! -d "$kd" ]; then
    echo "Usage: $0 dv_scs_keys"
    echo "Renames dv_scs_keys key directory to new names."
    exit 1
fi

if [ ! -d "$kd/boot-blobs" ] &&
        [ ! -d "$kd/fip" ] &&
        [ ! -d "$kd/root" ]; then
    echo "Error: Unable to find boot-blobs, fip or root directories"
    exit 1
fi

if [ -d "$kd/boot-blobs/aes" ]; then
    for part in "$kd"/boot-blobs/aes/*; do
        part="${part%/}"
        mv_if_exists "$part/rootrsa-0" "$part/trustchain-0"
        mv_if_exists "$part/rootrsa-1" "$part/trustchain-1"
        mv_if_exists "$part/rootrsa-2" "$part/trustchain-2"
        mv_if_exists "$part/rootrsa-3" "$part/trustchain-3"
    done
fi

if [ -d "$kd/boot-blobs/rsa" ]; then
    for part in "$kd"/boot-blobs/rsa/*; do
        part="${part%/}"
        mv_if_exists "$part/rootrsa-0" "$part/trustchain-0"
        mv_if_exists "$part/rootrsa-1" "$part/trustchain-1"
        mv_if_exists "$part/rootrsa-2" "$part/trustchain-2"
        mv_if_exists "$part/rootrsa-3" "$part/trustchain-3"

        mv_if_exists "$part/trustchain-0/nonce/device-lvl1rsa-nonce.bin" "$part/trustchain-0/nonce/lvl1key-nonce.bin"
        mv_if_exists "$part/trustchain-1/nonce/device-lvl1rsa-nonce.bin" "$part/trustchain-1/nonce/lvl1key-nonce.bin"
        mv_if_exists "$part/trustchain-2/nonce/device-lvl1rsa-nonce.bin" "$part/trustchain-2/nonce/lvl1key-nonce.bin"
        mv_if_exists "$part/trustchain-3/nonce/device-lvl1rsa-nonce.bin" "$part/trustchain-3/nonce/lvl1key-nonce.bin"

        mv_if_exists "$part/trustchain-0/nonce/device-lvl2rsa-nonce.bin" "$part/trustchain-0/nonce/lvl2key-nonce.bin"
        mv_if_exists "$part/trustchain-1/nonce/device-lvl2rsa-nonce.bin" "$part/trustchain-1/nonce/lvl2key-nonce.bin"
        mv_if_exists "$part/trustchain-2/nonce/device-lvl2rsa-nonce.bin" "$part/trustchain-2/nonce/lvl2key-nonce.bin"
        mv_if_exists "$part/trustchain-3/nonce/device-lvl2rsa-nonce.bin" "$part/trustchain-3/nonce/lvl2key-nonce.bin"
    done
fi
if [ -d "$kd/boot-blobs/template" ]; then
    for part in "$kd"/boot-blobs/template/*; do
        part="${part%/}"
        mv_if_exists "$part/rootrsa-0" "$part/trustchain-0"
        mv_if_exists "$part/rootrsa-1" "$part/trustchain-1"
        mv_if_exists "$part/rootrsa-2" "$part/trustchain-2"
        mv_if_exists "$part/rootrsa-3" "$part/trustchain-3"
    done
fi
if [ -d "$kd/fip/rsa" ]; then
    for part in "$kd"/fip/rsa/*; do
        part="${part%/}"
        mkdir -p "$part/trustchain-0/epk"
        mkdir -p "$part/trustchain-1/epk"
        mkdir -p "$part/trustchain-2/epk"
        mkdir -p "$part/trustchain-3/epk"
        mkdir "$part/trustchain-0/key"
        mkdir "$part/trustchain-1/key"
        mkdir "$part/trustchain-2/key"
        mkdir "$part/trustchain-3/key"
        mkdir "$part/trustchain-0/nonce"
        mkdir "$part/trustchain-1/nonce"
        mkdir "$part/trustchain-2/nonce"
        mkdir "$part/trustchain-3/nonce"

        for blx in bl30 bl31 bl32 bl33 bl40 krnl; do
            part_only=$(basename $part)
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-0/epk/lvl2cert-epks.bin" "$part/trustchain-0/epk/$blx-lvl3cert-epks.bin"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-1/epk/lvl2cert-epks.bin" "$part/trustchain-1/epk/$blx-lvl3cert-epks.bin"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-2/epk/lvl2cert-epks.bin" "$part/trustchain-2/epk/$blx-lvl3cert-epks.bin"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-3/epk/lvl2cert-epks.bin" "$part/trustchain-3/epk/$blx-lvl3cert-epks.bin"

            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-0/key/level-2-rsa-priv.pem" "$part/trustchain-0/key/$blx-level-3-rsa-priv.pem"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-0/key/level-2-rsa-pub.pem"  "$part/trustchain-0/key/$blx-level-3-rsa-pub.pem"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-1/key/level-2-rsa-priv.pem" "$part/trustchain-1/key/$blx-level-3-rsa-priv.pem"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-1/key/level-2-rsa-pub.pem"  "$part/trustchain-1/key/$blx-level-3-rsa-pub.pem"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-2/key/level-2-rsa-priv.pem" "$part/trustchain-2/key/$blx-level-3-rsa-priv.pem"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-2/key/level-2-rsa-pub.pem"  "$part/trustchain-2/key/$blx-level-3-rsa-pub.pem"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-3/key/level-2-rsa-priv.pem" "$part/trustchain-3/key/$blx-level-3-rsa-priv.pem"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-3/key/level-2-rsa-pub.pem"  "$part/trustchain-3/key/$blx-level-3-rsa-pub.pem"

            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-0/nonce/lvl2key-nonce.bin" "$part/trustchain-0/nonce/$blx-lvl3key-nonce.bin"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-1/nonce/lvl2key-nonce.bin" "$part/trustchain-1/nonce/$blx-lvl3key-nonce.bin"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-2/nonce/lvl2key-nonce.bin" "$part/trustchain-2/nonce/$blx-lvl3key-nonce.bin"
            ln -srf "$kd/boot-blobs/rsa/$part_only/trustchain-3/nonce/lvl2key-nonce.bin" "$part/trustchain-3/nonce/$blx-lvl3key-nonce.bin"
        done
    done
fi
if [ -d "$kd/fip/template" ]; then
    for part in "$kd"/fip/template/*; do
        part="${part%/}"
        mv_if_exists "$part/rootrsa-0" "$part/trustchain-0"
        mv_if_exists "$part/rootrsa-1" "$part/trustchain-1"
        mv_if_exists "$part/rootrsa-2" "$part/trustchain-2"
        mv_if_exists "$part/rootrsa-3" "$part/trustchain-3"
    done
fi

if [ -d "$kd/root/rsa" ]; then
    for part in "$kd"/root/rsa/*; do
        part="${part%/}"
        mv_if_exists "$part/nonce/rootrsa-0-nonce.bin" "$part/nonce/rootkey-0-nonce.bin"
        mv_if_exists "$part/nonce/rootrsa-1-nonce.bin" "$part/nonce/rootkey-1-nonce.bin"
        mv_if_exists "$part/nonce/rootrsa-2-nonce.bin" "$part/nonce/rootkey-2-nonce.bin"
        mv_if_exists "$part/nonce/rootrsa-3-nonce.bin" "$part/nonce/rootkey-3-nonce.bin"

        mv_if_exists "$part/roothash/hash-device-rootcert.bin" "$part/roothash/hash-device-rootcert-rsa.bin"
    done
fi
