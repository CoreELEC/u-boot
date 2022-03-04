#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

# usage:./scripts/build_all.sh at rtos sdk root dir

LOCAL_DOC_PATH="$PWD/output/docs/html"
REMOTE_DOC_PATH="ftp://platform:platform@10.68.11.163:2222/Documents/Ecosystem/RTOS/rtos-sdk/"

# Build and upload document
update_docoment()
{
	find -type f | while read filename
	do
		curl -s --ftp-create-dirs -T $filename $REMOTE_DOC_PATH/$filename
		if [ $? -ne 0 ]; then
			return 1;
		fi
	done
}

if [[ "$SUBMIT_TYPE" == "daily" ]]; then
	make docs
	if [ -d $LOCAL_DOC_PATH ]; then
		pushd $LOCAL_DOC_PATH > /dev/null
		update_docoment
		if [ $? -ne 0 ]; then
			echo "Failed to update document"
		else
			echo "Document updated!"
		fi
		popd > /dev/null
	else
		echo "$LOCAL_DOC_PATH not exist!"
	fi
fi

BUILD_DATE=`date +%F`
FIRMWARE_SERVER=firmware.amlogic.com

publish_firmware()
{
	LOCAL_FIRMWARE_PATH=output/$BOARD-$PRODUCT
	REMOTE_FIRMWARE_PATH=/data/shanghai/image/RTOS/$BUILD_DATE/$BOARD-$PRODUCT

	if [ -d $LOCAL_FIRMWARE_PATH ]
	then

		ssh -n autobuild@$FIRMWARE_SERVER "mkdir -p $REMOTE_FIRMWARE_PATH"
		if [ $? -ne 0 ]
		then
			echo "Failed to create remote path! $REMOTE_FIRMWARE_PATH"
			exit 1
		else
			echo "Remote path: $REMOTE_FIRMWARE_PATH"
		fi
		scp build.log autobuild@$FIRMWARE_SERVER:$REMOTE_FIRMWARE_PATH
		pushd $LOCAL_FIRMWARE_PATH > /dev/null
		tar -cJf $KERNEL.tar.xz $KERNEL/$KERNEL.*
		LOCAL_FILES="manifest.xml $KERNEL.tar.xz"
		scp $LOCAL_FILES autobuild@$FIRMWARE_SERVER:$REMOTE_FIRMWARE_PATH
		scp -r images autobuild@$FIRMWARE_SERVER:$REMOTE_FIRMWARE_PATH
		popd > /dev/null
		echo "Publish success."
	else
		echo "No local path! $LOCAL_FIRMWARE_PATH"
	fi
}

# Manually cherry pick patches
./scripts/cherry_pick.sh

# Build all projects
BUILD_COMBINATION="$PWD/scripts/build_combination.txt"

i=0
while IFS= read -r LINE; do
	[[ "$i" -ne 0 ]] && echo ""
	i=$((i+1))
	source scripts/env.sh $LINE
	[ "$?" -ne 0 ] && echo "Failed to source scripts/env.sh!" && exit 1
	make distclean
	[ "$?" -ne 0 ] && echo "Failed to make distclean!" && exit 2
	make
	[ "$?" -ne 0 ] && echo "Failed to make!" && exit 3
	if [[ "$SUBMIT_TYPE" == "daily" ]]; then
		publish_firmware
		[ "$?" -ne 0 ] && echo "Failed to source scripts/scp.sh!" && exit 4
	fi
done < "$BUILD_COMBINATION"

echo "Build completed!"
