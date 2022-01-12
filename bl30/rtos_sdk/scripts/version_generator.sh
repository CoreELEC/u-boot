#!/bin/sh
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

PRODUCT_PATH=$1

if [ -d $PRODUCT_PATH ];then
	GET_DATE=$(date +%F)
	GET_TIME=$(date +%H:%M:%S)
	COMMIT_ID=$(cd ${PRODUCT_PATH} && git log -1 2>/dev/null | head -n 1 | cut -d " " -f 2)
	BRANCH=$(cd ${PRODUCT_PATH} && git branch 2>/dev/null | grep "\*" | cut -d " " -f 2)

	PRODUCT_VERSION_MSG="VER_"$BRANCH"_"$COMMIT_ID"_"$GET_TIME"_"$GET_DATE
	echo $PRODUCT_VERSION_MSG
else
	echo "product path:$PRODUCT_PATH is not exist,plese check it..."
fi
