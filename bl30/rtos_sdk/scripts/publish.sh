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
LOCAL_OUTPUT_PATH=$OUTPUT_DIR

BUILD_DATE=$(date +%F)
LATEST_REMOTE_PATH=/data/shanghai/image/RTOS/latest
REMOTE_PATH=/data/shanghai/image/RTOS/$BUILD_DATE
REMOTE_IMAGES_PATH=$REMOTE_PATH/images
REMOTE_PACKAGES_PATH=$REMOTE_PATH/packages

FIRMWARE_ACCOUNT=autobuild
FIRMWARE_SERVER=firmware.amlogic.com

make_image() {
	mkimage -A $ARCH -O u-boot -T standalone -C none -a 0x1000 -e 0x1000 -n rtos -d $LOCAL_IMAGE_PATH/images/freertos-signed.bin $LOCAL_IMAGE_PATH/images/rtos-uImage
}

publish_images() {
	LOCAL_IMAGE_PATH=$LOCAL_OUTPUT_PATH/$ARCH-$BOARD-$PRODUCT
	REMOTE_IMAGE_PATH=$REMOTE_IMAGES_PATH/$ARCH-$BOARD-$PRODUCT

	if [ -d $LOCAL_IMAGE_PATH ]; then
		ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "mkdir -p $REMOTE_IMAGE_PATH"
		if [ $? -ne 0 ]; then
			echo "Failed to create remote image path! $REMOTE_IMAGE_PATH"
			exit 1
		else
			echo "Remote image path: $REMOTE_IMAGE_PATH"
		fi
		pushd $LOCAL_IMAGE_PATH >/dev/null
		tar -cJf $KERNEL.tar.xz $KERNEL/$KERNEL.*
		scp $KERNEL.tar.xz $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_IMAGE_PATH
		scp images/* $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_IMAGE_PATH
		popd >/dev/null
		echo "Images publish done."
	else
		echo "No local image path! $LOCAL_IMAGE_PATH"
	fi
}

post_publish_images() {
	ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "mkdir -p $REMOTE_IMAGES_PATH"
	if [ $? -ne 0 ]; then
		echo "Failed to create remote image path! $REMOTE_IMAGES_PATH"
		exit 1
	else
		echo "Remote image path: $REMOTE_IMAGES_PATH"
	fi

	LOCAL_FILES="$LOCAL_OUTPUT_PATH/build.log"
	[ -z "$MANIFEST" ] && MANIFEST="$OUTPUT_DIR/manifest.xml"
	[ -f $MANIFEST ] && LOCAL_FILES+=" $MANIFEST"
	[ -z "$JENKINS_TRIGGER" ] && JENKINS_TRIGGER="$OUTPUT_DIR/jenkins_trigger.txt"
	[ -f $JENKINS_TRIGGER ] && LOCAL_FILES+=" $JENKINS_TRIGGER"
	scp $LOCAL_FILES $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_IMAGES_PATH

	ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "rm -f $LATEST_REMOTE_PATH"
	ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "ln -s $REMOTE_PATH $LATEST_REMOTE_PATH"
	echo "Post images publish done."
}

publish_packages() {
	LOCAL_PACKAGE_PATH=$LOCAL_OUTPUT_PATH/packages/$CURRENT_PRODUCTS_DIR_NAME
	REMOTE_PACKAGE_PATH=$REMOTE_PACKAGES_PATH/$CURRENT_PRODUCTS_DIR_NAME

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
	ssh -n $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER "mkdir -p $REMOTE_PACKAGES_PATH"
	if [ $? -ne 0 ]; then
		echo "Failed to create remote package path! $REMOTE_PACKAGES_PATH"
		exit 1
	else
		echo "Remote package path: $REMOTE_PACKAGES_PATH"
	fi
	LOCAL_FILES="$LOCAL_OUTPUT_PATH/build.log $LOCAL_OUTPUT_PATH/manifest.xml"
	scp $LOCAL_FILES $FIRMWARE_ACCOUNT@$FIRMWARE_SERVER:$REMOTE_PACKAGES_PATH
	echo "Post packages publish done."
}
