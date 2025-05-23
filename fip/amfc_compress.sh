#!/bin/bash

set -e
# set -x

#
# Settings
#
BASEDIR_TOP=$1
SOC=$2
INPUT=$3
OUTPUT=$4

TOOLBIN=${BASEDIR_TOP}/tools/zstd
AMFC_ZSTD_HDR=${BASEDIR_TOP}/amfc_zstd_hdr.bin

COMPRESS_ORG=${BASEDIR_TOP}/${INPUT}.org
COMPRESS_ZSTD=${BASEDIR_TOP}/${INPUT}.zstd

mv -f $INPUT $COMPRESS_ORG

${TOOLBIN} ${COMPRESS_ORG} -9 -o ${COMPRESS_ZSTD}
bin_org_size=`stat -c %s ${COMPRESS_ORG}`
bin_zstd_size=`stat -c %s ${COMPRESS_ZSTD}`
if [ "$SOC" == "t6d" ]; then
    printf "%s" "@ZSTD" > "${AMFC_ZSTD_HDR}"
else
    printf "%s" "ZSTD" > "${AMFC_ZSTD_HDR}"
fi

printf "%02x%02x%02x%02x" $[(bin_org_size) & 0xff] \
$[((bin_org_size) >> 8) & 0xff] $[((bin_org_size) >> 16) & 0xff] \
$[((bin_org_size) >> 24) & 0xff] | xxd -r -ps >>  ${AMFC_ZSTD_HDR}

printf "%02x%02x%02x%02x" $[(bin_zstd_size) & 0xff] \
$[((bin_zstd_size) >> 8) & 0xff] $[((bin_zstd_size) >> 16) & 0xff] \
$[((bin_zstd_size) >> 24) & 0xff] | xxd -r -ps >>  ${AMFC_ZSTD_HDR}

cat ${AMFC_ZSTD_HDR} ${COMPRESS_ZSTD} > ${OUTPUT}
rm ${AMFC_ZSTD_HDR} -f

# vim: set tabstop=2 expandtab shiftwidth=2:
