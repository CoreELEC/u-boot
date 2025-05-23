#!/bin/bash

set -e
# set -x

#
# Variables
#

EXEC_BASEDIR=$(dirname $(readlink -f $0))
ACPU_IMAGETOOL=${EXEC_BASEDIR}/../binary-tool/acpu-imagetool

BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

#
# Settings
#

BASEDIR_TEMPLATE="${BASEDIR_TOP}/templates"

BASEDIR_PAYLOAD=$2

BASEDIR_NONCE="./nonce"

CHIPSET_NAME=$4
KEY_TYPE=$5
SOC=$6
DV_SIGNING_SCHEME=$7
CS_SIGNING_SCHEME=$8

BASEDIR_AESKEY_PROT_BL2="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl2/aes/${CHIPSET_NAME}"
BASEDIR_RSAKEY_LVLX_BL2="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl2/$CS_SIGNING_SCHEME/${CHIPSET_NAME}"

BASEDIR_AESKEY_PROT_BL31="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl31/aes/${CHIPSET_NAME}"
BASEDIR_RSAKEY_LVLX_BL31="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl31/$CS_SIGNING_SCHEME/${CHIPSET_NAME}"

BASEDIR_AESKEY_PROT_BL32="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl32/aes/${CHIPSET_NAME}"
BASEDIR_RSAKEY_LVLX_BL32="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl32/$CS_SIGNING_SCHEME/${CHIPSET_NAME}"

BASEDIR_AESKEY_PROT_BL40="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl40/aes/${CHIPSET_NAME}"
BASEDIR_RSAKEY_LVLX_BL40="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl40/$CS_SIGNING_SCHEME/${CHIPSET_NAME}"

BASEDIR_OUTPUT_BLOB=$3
postfix=.signed
#
# Arguments
#

BLOB_NAME=$1
_BASEDIR_AESKEY_PROT_DIR="BASEDIR_AESKEY_PROT_BL${BLOB_NAME}"
BASEDIR_AESKEY_PROT_DIR=${!_BASEDIR_AESKEY_PROT_DIR}
_BASEDIR_RSAKEY_LVLX_DIR="BASEDIR_RSAKEY_LVLX_BL${BLOB_NAME}"
BASEDIR_RSAKEY_LVLX_DIR=${!_BASEDIR_RSAKEY_LVLX_DIR}

EXEC_ARGS="${EXEC_ARGS}"

### Input: payload ###
EXEC_ARGS="${EXEC_ARGS} --infile-bl${BLOB_NAME}-payload=${BASEDIR_PAYLOAD}/bl${BLOB_NAME}-payload.bin"

### Input: Chipset Level-1/2 Private RSA keys

EXEC_ARGS="${EXEC_ARGS} --infile-signkey-bl${BLOB_NAME}-chipset-lvl3=${BASEDIR_RSAKEY_LVLX_DIR}/bl${BLOB_NAME}-level-3-rsa-priv.pem"
if [ "$CS_SIGNING_SCHEME" == "rsa-mldsa" ]; then
  EXEC_ARGS="${EXEC_ARGS} --infile-signkey-bl${BLOB_NAME}-chipset-lvl3-pqc=${BASEDIR_RSAKEY_LVLX_DIR}/bl${BLOB_NAME}-level-3-mldsa-draft1-priv.pem"
fi

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

### Input: nonce for binary protection ###
#EXEC_ARGS="${EXEC_ARGS} --infile-nonce-blob-bl${BLOB_NAME}=${BASEDIR_NONCE}/chipset/blob/blob-bl${BLOB_NAME}-nonce.bin"

### Input: pre-generated ProtKey for payload
EXEC_ARGS="${EXEC_ARGS} --infile-aes256-bl${BLOB_NAME}-payload=${BASEDIR_AESKEY_PROT_DIR}/genkey-prot-bl${BLOB_NAME}.bin"

### Features, flags and switches ###

### Output: blobs ###
EXEC_ARGS="${EXEC_ARGS} --outfile-blob-bl${BLOB_NAME}=${BASEDIR_OUTPUT_BLOB}/blob-bl${BLOB_NAME}.bin${postfix}"

### full Device FIP Header
EXEC_ARGS="${EXEC_ARGS} --header-layout=mini"
EXEC_ARGS="${EXEC_ARGS} --size-payload-bl30=90112"

#echo ${EXEC_ARGS}

#
# Main
#

set -x

${ACPU_IMAGETOOL} \
        create-device-fip \
        ${EXEC_ARGS}

# vim: set tabstop=2 expandtab shiftwidth=2:
