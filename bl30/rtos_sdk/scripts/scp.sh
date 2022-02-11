#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

BUILD_DATE=`date +%F`
TARGET_DIR=images
PUBLISH_SERVER=firmware.amlogic.com
SERVER_ROOT_DIR=/data/shanghai/image/RTOS
PRODUCT_BUILD_DIR=$BOARD-$PRODUCT
SRC_DIR=output/$PRODUCT_BUILD_DIR/$TARGET_DIR
HTTP_DEST_DIR=$BUILD_DATE/$PRODUCT_BUILD_DIR
SCP_DEST_DIR=$SERVER_ROOT_DIR/$HTTP_DEST_DIR

if [ -d $SRC_DIR ]
then

	ssh -n autobuild@$PUBLISH_SERVER "mkdir -p $SCP_DEST_DIR"
	if [ $? -ne 0 ]
	then
		echo "Failed to create publish path! $SCP_DEST_DIR"
		exit 1
	else
		echo "Publish path: $SCP_DEST_DIR"
	fi
	scp -r $SRC_DIR autobuild@$PUBLISH_SERVER:$SCP_DEST_DIR
	echo "Publish success."
else
	echo "output target directory doesn't exist!"
fi
