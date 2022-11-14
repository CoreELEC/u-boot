#!/bin/sh
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

RTOS_SDK_VERSION_FILE=$1
RTOS_SDK_MANIFEST_FILE=$2
RTOS_SDK_MANIFEST_OLD_FILE=$3

echo "#ifndef __SDK_VER_H__" > $RTOS_SDK_VERSION_FILE
echo "#define __SDK_VER_H__" >> $RTOS_SDK_VERSION_FILE
echo "" >> $RTOS_SDK_VERSION_FILE

echo -e "#include <stdio.h>\n" >> $RTOS_SDK_VERSION_FILE

COMPILE_TIME=`date +%F" "%T`
echo "#define CONFIG_BOARD_NAME \"$BOARD\"" >> $RTOS_SDK_VERSION_FILE
echo "#define CONFIG_PRODUCT_NAME \"$PRODUCT\"" >> $RTOS_SDK_VERSION_FILE
echo "#define CONFIG_COMPILE_TIME \"$COMPILE_TIME\"" >> $RTOS_SDK_VERSION_FILE
# Get SDK_VERSION
pattern="revision="
keyline=`grep 'default .* revision' $RTOS_SDK_MANIFEST_FILE`
for keyword in $keyline; do
	let i++
	if [[ $keyword == $pattern* ]]; then
		SDK_VERSION=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`
		break;
	fi
done
echo "#define CONFIG_VERSION_STRING \"$SDK_VERSION\"" >> $RTOS_SDK_VERSION_FILE
echo "" >> $RTOS_SDK_VERSION_FILE

echo "const char* version_map[] = {" >> $RTOS_SDK_VERSION_FILE
printf "	\"[%-7s %-12s %-12s]\",\n" "cm_hash" "remote_name" "branch" \
	>> $RTOS_SDK_VERSION_FILE
GIT_LISTS=$(find ! -path "*/.repo/*" -name ".git")
for PRODUCT_PATH in $GIT_LISTS;
do
	if [ -d $PRODUCT_PATH ];then
		#echo $PRODUCT_PATH
		GIT_REMOTE_NAME="$(cd $PRODUCT_PATH && git remote -v 2>/dev/null \
			| head -n 1 | cut -d " " -f 1)"
		GIT_NAME=${GIT_REMOTE_NAME##*rtos_sdk/}
		GIT_1ST_NAME="$(echo ${GIT_NAME} | cut -d / -f 1)"
		GIT_2ND_NAME=${GIT_REMOTE_NAME##*/}
		COMMIT_HASH="$(cd $PRODUCT_PATH && git log --oneline -1 2>/dev/null \
			| head -n 1 | cut -d " " -f 1)"
		BRANCH=$(cd $PRODUCT_PATH && git branch 2>/dev/null | grep "\*" | cut -d " " -f 2)
		BRANCH=${BRANCH##*"/"}
		if [ "${GIT_1ST_NAME}"x = "${GIT_2ND_NAME}"x ];then
			printf "	\"[%-7s %-12s %-12s]\",\n" $COMMIT_HASH \
				${GIT_1ST_NAME:0:12} ${BRANCH:0:12} >> $RTOS_SDK_VERSION_FILE
		else
			printf "	\"[%-7s %-4s %-7s %-12s]\",\n" $COMMIT_HASH \
				${GIT_1ST_NAME:0:4} ${GIT_2ND_NAME:0:7} ${BRANCH:0:12} \
				>> $RTOS_SDK_VERSION_FILE
		fi
	else
		echo "product path:$PRODUCT_PATH is not exist,please check it..."
	fi
done
echo "};" >> $RTOS_SDK_VERSION_FILE

echo "#ifdef CONFIG_VERSION_FULL_INFO" >> $RTOS_SDK_VERSION_FILE
echo "#define OUTPUT_VERSION_FULL_INFO() \\
do{ \\
	for (int i = 0; i < sizeof(version_map) / sizeof(const char*); i++) \\
		printf(\"%s\\n\", version_map[i]); \\
} \\
while(0)"  >> $RTOS_SDK_VERSION_FILE
echo "#else" >> $RTOS_SDK_VERSION_FILE
echo -e "#define OUTPUT_VERSION_FULL_INFO()">> $RTOS_SDK_VERSION_FILE
echo "#endif" >> $RTOS_SDK_VERSION_FILE

echo "" >> $RTOS_SDK_VERSION_FILE
echo "#endif" >> $RTOS_SDK_VERSION_FILE
