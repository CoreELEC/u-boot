#!/bin/bash

set -e
# set -x

#
# Variables
#

EXEC_BASEDIR=$(dirname $(readlink -f $0))
CP=cp

BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

#
# Settings
#

BASEDIR_TEMPLATE=$1

BASEDIR_PAYLOAD=$2

BASEDIR_INPUT_BLOB=$3

BASEDIR_OUTPUT=$4

if [ ".fastboot" == "$5" ]; then
	CHIPSET_VARIANT_SUFFIX=$5
	CHIPSET_VARIANT_MIN_SUFFIX=$6
else
	CHIPSET_VARIANT_SUFFIX=""
	CHIPSET_VARIANT_MIN_SUFFIX=$5
fi

if [ "" != "${CHIPSET_VARIANT_MIN_SUFFIX}" ] && [ ".fastboot" == "${CHIPSET_VARIANT_SUFFIX}" ]; then
	ACPU_IMAGETOOL=${EXEC_BASEDIR}/../binary-tool/acpu-imagetool-fastboot-oversea
elif [ "" == "${CHIPSET_VARIANT_MIN_SUFFIX}" ] && [ ".fastboot" == "${CHIPSET_VARIANT_SUFFIX}" ]; then
	ACPU_IMAGETOOL=${EXEC_BASEDIR}/../binary-tool/acpu-imagetool-fastboot
else
	ACPU_IMAGETOOL=${EXEC_BASEDIR}/../binary-tool/acpu-imagetool
fi

#
# Arguments
#

EXEC_ARGS="${EXEC_ARGS}"

### Input: template ###
EXEC_ARGS="${EXEC_ARGS} --infile-template-chipset-fip-header=${BASEDIR_TEMPLATE}/device-fip-header.bin"

### Input: payload ###
EXEC_ARGS="${EXEC_ARGS} --infile-bl30-payload=${BASEDIR_PAYLOAD}/bl30-payload.bin"
EXEC_ARGS="${EXEC_ARGS} --infile-bl33-payload=${BASEDIR_PAYLOAD}/bl33-payload.bin"

### Input: chipset blobs ###

EXEC_ARGS="${EXEC_ARGS} --infile-blob-bl40=${BASEDIR_INPUT_BLOB}/blob-bl40${CHIPSET_VARIANT_SUFFIX}.bin.signed"
EXEC_ARGS="${EXEC_ARGS} --infile-blob-bl31=${BASEDIR_INPUT_BLOB}/blob-bl31${CHIPSET_VARIANT_SUFFIX}.bin.signed"
EXEC_ARGS="${EXEC_ARGS} --infile-blob-bl32=${BASEDIR_INPUT_BLOB}/blob-bl32${CHIPSET_VARIANT_MIN_SUFFIX}${CHIPSET_VARIANT_SUFFIX}.bin.signed"

### Features, flags and switches ###

### Output: Device FIP ###
EXEC_ARGS="${EXEC_ARGS} --outfile-device-fip=${BASEDIR_OUTPUT}/device-fip.bin.signed"

if [ "" == "${CHIPSET_VARIANT_MIN_SUFFIX}" ] && [ ".fastboot" == "${CHIPSET_VARIANT_SUFFIX}" ]; then
	EXEC_ARGS="${EXEC_ARGS}	--header-layout=full"
fi
#echo ${EXEC_ARGS}

#
# Main
#

set -x

${ACPU_IMAGETOOL} \
        create-device-fip \
        ${EXEC_ARGS}

# vim: set tabstop=2 expandtab shiftwidth=2:
