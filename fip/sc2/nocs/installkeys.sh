#!/bin/bash

set -eu -o pipefail

trap 'echo "ERROR: ${BASH_SOURCE[0]}: line: $LINENO";' ERR

# Uncomment following line for debugging
# set -x

#
# Main
#

set -x
INSTALLDIR=$1
DEVICE_KEYS=../../../bl33/v2019/board/amlogic/$1/device-keys
if [ -d ${DEVICE_KEYS} ]; then
	rm -rf ${DEVICE_KEYS}
fi
mkdir -p ${DEVICE_KEYS}/
cp -r ./stage-3a-stbm-generate-keysets/output/data-stbm/outdir/* ../../../bl33/v2019/board/amlogic/$1/device-keys

# vim: set syntax=sh filetype=sh tabstop=4 expandtab shiftwidth=4 softtabstop=-1:
