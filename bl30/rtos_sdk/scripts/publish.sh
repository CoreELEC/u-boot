#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

# Build and upload document
publish_docoment() {
	REMOTE_DOC_PATH="ftp://platform:platform@10.68.11.163:2222/Documents/Ecosystem/RTOS/rtos-sdk/"

	find -type f | while read filename; do
		curl -s --ftp-create-dirs -T $filename $REMOTE_DOC_PATH/$filename
		if [ $? -ne 0 ]; then
			return 1
		fi
	done
}

[ -z "$OUTPUT_DIR" ] && OUTPUT_DIR=$PWD/output
LOCAL_PATH=$OUTPUT_DIR

BUILD_DATE=$(date +%F)
REMOTE_BASE=/data/shanghai/image/RTOS
if [[ "$SUBMIT_TYPE" == "daily" || "$SUBMIT_TYPE" == "release" || "$SUBMIT_TYPE" == "patch" ]]; then
	REMOTE_PATH=$REMOTE_BASE/$SUBMIT_TYPE/$BUILD_DATE
else
	REMOTE_PATH=$REMOTE_BASE/$SUBMIT_TYPE
fi
LATEST_PATH=$REMOTE_BASE/$SUBMIT_TYPE/latest

FIRMWARE_ACCOUNT=autobuild
FIRMWARE_SERVER=firmware.amlogic.com

make_image() {
	LOCAL_IMAGE_PATH=$LOCAL_PATH/$ARCH-$BOARD-$PRODUCT
	mkimage -A $ARCH -O u-boot -T standalone -C none -a 0x10000 -e 0x10000 -n rtos -d $LOCAL_IMAGE_PATH/images/freertos-signed.bin $LOCAL_IMAGE_PATH/images/rtos-uImage
}

publish_images() {
	LOCAL_IMAGE_PATH=$LOCAL_PATH/$ARCH-$BOARD-$PRODUCT
	REMOTE_IMAGE_PATH=$REMOTE_PATH/$ARCH-$BOARD-$PRODUCT
	echo "publish to $REMOTE_IMAGE_PATH"
	if [ -d $LOCAL_IMAGE_PATH ]; then
		ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "mkdir -p $REMOTE_IMAGE_PATH"

		if [ $? -ne 0 ]; then
			echo "Failed to create remote image path! $REMOTE_IMAGE_PATH"
			exit 1
		else
			echo "Remote image path: $REMOTE_IMAGE_PATH"
		fi
		pushd $LOCAL_IMAGE_PATH >/dev/null
		scp images/* $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_IMAGE_PATH
		popd >/dev/null
		echo "Images publish done."
	else
		echo "No local image path! $LOCAL_IMAGE_PATH"
	fi
}

post_publish_images() {
	ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "mkdir -p $REMOTE_PATH"
	if [ $? -ne 0 ]; then
		echo "Failed to create remote image path! $REMOTE_PATH"
		exit 1
	else
		echo "Remote image path: $REMOTE_PATH"
	fi

	LOCAL_FILES="$LOCAL_PATH/build.log"
	[ -z "$MANIFEST" ] && MANIFEST="$OUTPUT_DIR/manifest.xml"
	[ -f $MANIFEST ] && LOCAL_FILES+=" $MANIFEST"
	[ -z "$JENKINS_TRIGGER" ] && JENKINS_TRIGGER="$OUTPUT_DIR/jenkins_trigger.txt"
	[ -f $JENKINS_TRIGGER ] && LOCAL_FILES+=" $JENKINS_TRIGGER"
	scp $LOCAL_FILES $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_PATH

	ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "rm -f $LATEST_PATH"
	ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "ln -s $REMOTE_PATH $LATEST_PATH"
	echo "Post images publish done."
}

publish_packages() {
	LOCAL_PACKAGE_PATH=$LOCAL_PATH/packages/$CURRENT_PRODUCTS_DIR_NAME
	REMOTE_PACKAGE_PATH=$REMOTE_PATH/$CURRENT_PRODUCTS_DIR_NAME

	if [ -d $LOCAL_PACKAGE_PATH ]; then
		ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "mkdir -p $REMOTE_PACKAGE_PATH"
		if [ $? -ne 0 ]; then
			echo "Failed to create remote package path! $REMOTE_PACKAGE_PATH"
			exit 1
		else
			echo "Remote package path: $REMOTE_PACKAGE_PATH"
		fi
		pushd $LOCAL_PACKAGE_PATH >/dev/null

		scp -r . $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_PACKAGE_PATH
		popd >/dev/null
		echo "Packages publish done."
	else
		echo "No local package path! $LOCAL_PACKAGE_PATH"
	fi
}

post_publish_packages() {
	ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "mkdir -p $REMOTE_PATH"
	if [ $? -ne 0 ]; then
		echo "Failed to create remote package path! $REMOTE_PATH"
		exit 1
	else
		echo "Remote package path: $REMOTE_PATH"
	fi
	LOCAL_FILES="$LOCAL_PATH/build.log $LOCAL_PATH/manifest.xml"
	scp $LOCAL_FILES $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_PATH
	echo "Post packages publish done."
}

show_download_url() {
	WEB_PATH=`echo ${LATEST_PATH#*data}`
	DOWNLOAD_URL="http://$FIRMWARE_SERVER$WEB_PATH"
	echo "<Download Address> $DOWNLOAD_URL"
}
