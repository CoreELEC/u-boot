#!/bin/bash

set -e
# set -x

#
# Variables
#

EXEC_BASEDIR=$(dirname $(readlink -f $0))
ACPU_IMAGETOOL=${EXEC_BASEDIR}/../binary-tool/acpu-imagetool
CP=cp

BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

#
# Settings
#

BASEDIR_TEMPLATE=$1

BASEDIR_PAYLOAD=$2

BASEDIR_INPUT_BLOB=$3

BASEDIR_OUTPUT=$4

CHIPSET_VARIANT_SUFFIX=$5

#
# Arguments
#

EXEC_ARGS="${EXEC_ARGS}"

### Input: template ###
template_ext=".${DV_SIGNING_SCHEME}.${CS_SIGNING_SCHEME}"
EXEC_ARGS="${EXEC_ARGS} --infile-template-chipset-fip-header=${BASEDIR_TEMPLATE}/device-fip-header.bin${template_ext}"

### Input: payload ###
EXEC_ARGS="${EXEC_ARGS} --infile-bl30-payload=${BASEDIR_PAYLOAD}/bl30-payload.bin"
EXEC_ARGS="${EXEC_ARGS} --infile-bl33-payload=${BASEDIR_PAYLOAD}/bl33-payload.bin"

### Input: chipset blobs ###

EXEC_ARGS="${EXEC_ARGS} --infile-blob-bl40=${BASEDIR_INPUT_BLOB}/blob-bl40.bin.signed"
EXEC_ARGS="${EXEC_ARGS} --infile-blob-bl31=${BASEDIR_INPUT_BLOB}/blob-bl31.bin.signed"
EXEC_ARGS="${EXEC_ARGS} --infile-blob-bl32=${BASEDIR_INPUT_BLOB}/blob-bl32.bin.signed"

### Features, flags and switches ###
EXEC_ARGS="${EXEC_ARGS} --header-layout=mini"
EXEC_ARGS="${EXEC_ARGS} --size-payload-bl30=90112"
if [ "$CS_SIGNING_SCHEME" == "rsa" ]; then
  EXEC_ARGS="${EXEC_ARGS} --chipset-authen-algorithm=rsa,none"
elif [ "$CS_SIGNING_SCHEME" == "rsa-mldsa" ]; then
  EXEC_ARGS="${EXEC_ARGS} --chipset-authen-algorithm=rsa,mldsa-draft1"
elif [ "$CS_SIGNING_SCHEME" == "mldsa" ]; then
  EXEC_ARGS="${EXEC_ARGS} --chipset-authen-algorithm=none,mldsa-draft1"
fi
if [ "$DV_SIGNING_SCHEME" == "rsa" ]; then
  EXEC_ARGS="${EXEC_ARGS} --device-authen-algorithm=rsa,none"
elif [ "$DV_SIGNING_SCHEME" == "rsa-mldsa" ]; then
  EXEC_ARGS="${EXEC_ARGS} --device-authen-algorithm=rsa,mldsa-draft1"
elif [ "$DV_SIGNING_SCHEME" == "mldsa" ]; then
  EXEC_ARGS="${EXEC_ARGS} --device-authen-algorithm=none,mldsa-draft1"
fi

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
