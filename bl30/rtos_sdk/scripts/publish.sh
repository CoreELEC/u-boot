#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

LOCAL_DOC_PATH="$PWD/output/docs/html"
REMOTE_DOC_PATH="ftp://platform:platform@10.68.11.163:2222/Documents/Ecosystem/RTOS/rtos-sdk/"

# Build and upload document
publish_docoment() {
	find -type f | while read filename; do
		curl -s --ftp-create-dirs -T $filename $REMOTE_DOC_PATH/$filename
		if [ $? -ne 0 ]; then
			return 1
		fi
	done
}

BUILD_DATE=$(date +%F)
LOCAL_OUTPUT_PATH=output
FIRMWARE_ACCOUNT=autobuild
FIRMWARE_SERVER=firmware.amlogic.com

publish_image() {
	LOCAL_IMAGE_PATH=$LOCAL_OUTPUT_PATH/$ARCH-$BOARD-$PRODUCT
	REMOTE_IMAGE_PATH=/data/shanghai/image/RTOS/$BUILD_DATE/images/$ARCH-$BOARD-$PRODUCT

	if [ -d $LOCAL_IMAGE_PATH ]; then
		ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "mkdir -p $REMOTE_IMAGE_PATH"
		if [ $? -ne 0 ]; then
			echo "Failed to create remote image path! $REMOTE_IMAGE_PATH"
			exit 1
		else
			echo "Remote image path: $REMOTE_IMAGE_PATH"
		fi
		LOCAL_FILES="build.log $CURRENT_MANIFEST_FILE"
		scp $LOCAL_FILES $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_IMAGE_PATH
		pushd $LOCAL_IMAGE_PATH >/dev/null
		tar -cJf $KERNEL.tar.xz $KERNEL/$KERNEL.*
		scp $KERNEL.tar.xz $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_IMAGE_PATH
		scp -r images $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_IMAGE_PATH
		popd >/dev/null
		echo "Publish images success."
	else
		echo "No local image path! $LOCAL_IMAGE_PATH"
	fi
}

publish_package() {
	LOCAL_PACKAGE_PATH=$LOCAL_OUTPUT_PATH/package/images
	REMOTE_PACKAGE_PATH=/data/shanghai/image/RTOS/$BUILD_DATE/packages

	if [ -d $LOCAL_PACKAGE_PATH ]; then
		ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "mkdir -p $REMOTE_PACKAGE_PATH"
		if [ $? -ne 0 ]; then
			echo "Failed to create remote package path! $REMOTE_PACKAGE_PATH"
			exit 1
		else
			echo "Remote package path: $REMOTE_PACKAGE_PATH"
		fi
		LOCAL_FILES="build.log $CURRENT_MANIFEST_FILE"
		scp $LOCAL_FILES $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_PACKAGE_PATH
		pushd $LOCAL_PACKAGE_PATH >/dev/null
		scp -r . $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_PACKAGE_PATH
		popd >/dev/null
		echo "Publish packages success."
	else
		echo "No local package path! $LOCAL_PACKAGE_PATH"
	fi
}
