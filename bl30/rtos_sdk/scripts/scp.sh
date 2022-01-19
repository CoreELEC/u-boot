#!/bin/bash
BUILD_DATE=`date +%Y-%m-%d`
TARGET_DIR=images
PUBLISH_SERVER=firmware.amlogic.com
SERVER_ROOT_DIR=/data/shanghai/image/RTOS
PRODUCT_BUILD_DIR=$BOARD-$PRODUCT
SRC_DIR=output/$PRODUCT_BUILD_DIR/$TARGET_DIR
HTTP_DEST_DIR=$BUILD_DATE/$PRODUCT_BUILD_DIR
SCP_DEST_DIR=$SERVER_ROOT_DIR/$HTTP_DEST_DIR

if [ -d $SRC_DIR ]
then
	file_dirs=$SRC_DIR
fi

if [ -n "$file_dirs" ]
then
	echo "PUBLISH PATH : "$SCP_DEST_DIR
	ssh autobuild@$PUBLISH_SERVER   "mkdir -p $SCP_DEST_DIR"
	if [ $? -ne 0 ]
	then
        echo "ssh the publish server "$PUBLISH_SERVER" failed."
        exit 1
	fi
	scp -r $SRC_DIR autobuild@$PUBLISH_SERVER:$SCP_DEST_DIR
	echo "scp the $file_dirs success."
else
	echo "output target dir didn't exist"
fi
