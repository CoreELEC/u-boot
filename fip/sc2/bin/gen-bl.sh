#!/bin/bash

set -e
# set -x

#
# Variables
#

EXEC_BASEDIR=$(dirname $(readlink -f $0))
ACPU_IMAGETOOL=${EXEC_BASEDIR}/../acpu-imagetool
CP=cp

BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

#
# Settings
#

BASEDIR_TEMPLATE="${BASEDIR_TOP}/templates"

BASEDIR_PAYLOAD=$1

BASEDIR_INPUT_BLOB=$2

BASEDIR_OUTPUT=$3

#
# Arguments
#

BLOB_NAME=$1

EXEC_ARGS="${EXEC_ARGS}"

### Input: template ###
EXEC_ARGS="${EXEC_ARGS} --infile-template-chipset-fip-header=${BASEDIR_TEMPLATE}/device-fip-header.bin"

### Input: payload ###
EXEC_ARGS="${EXEC_ARGS} --infile-bl30-payload=${BASEDIR_PAYLOAD}/bl30-payload.bin"
EXEC_ARGS="${EXEC_ARGS} --infile-bl33-payload=${BASEDIR_PAYLOAD}/bl33-payload.bin"

### Input: chipset blobs ###

EXEC_ARGS="${EXEC_ARGS} --infile-blob-bl40=${BASEDIR_INPUT_BLOB}/blob-bl40.bin.signed"
EXEC_ARGS="${EXEC_ARGS} --infile-blob-bl31=${BASEDIR_INPUT_BLOB}/blob-bl31.bin.signed"
EXEC_ARGS="${EXEC_ARGS} --infile-blob-bl32=${BASEDIR_INPUT_BLOB}/blob-bl32.bin.signed"

### Features, flags and switches ###

### Output: Device FIP ###
EXEC_ARGS="${EXEC_ARGS} --outfile-device-fip=${BASEDIR_OUTPUT}/device-fip.bin.signed"

#echo ${EXEC_ARGS}

#
# Main
#

set -x

${ACPU_IMAGETOOL} \
        create-device-fip \
        ${EXEC_ARGS}

# vim: set tabstop=2 expandtab shiftwidth=2:
