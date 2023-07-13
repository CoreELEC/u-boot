#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

[ -z "$OUTPUT_DIR" ] && OUTPUT_DIR=$PWD/output
[ ! -d $OUTPUT_DIR ] && mkdir -p $OUTPUT_DIR

[ -z "$BUILD_LOG" ] && BUILD_LOG="$OUTPUT_DIR/build.log"

# Clear build.log
cat <<EOF > $BUILD_LOG
EOF

source scripts/publish.sh

if [[ "$SUBMIT_TYPE" == "daily" ]] || [[ "$SUBMIT_TYPE" == "release" ]]; then
	echo "======== Building document ========" | tee -a $BUILD_LOG
		make docs >> $BUILD_LOG 2>&1
		LOCAL_DOC_PATH="$OUTPUT_DIR/docs/arm64/html"
		if [ -d $LOCAL_DOC_PATH ]; then
			pushd $LOCAL_DOC_PATH >/dev/null
			publish_docoment
			if [ $? -ne 0 ]; then
				echo "Failed to update document!"
			else
				echo "Document updated."
			fi
			popd >/dev/null
		else
			echo "$LOCAL_DOC_PATH not exist!"
		fi
	echo -e "======== Done ========\n" | tee -a $BUILD_LOG
fi

echo "======== Building all projects ========" | tee -a $BUILD_LOG

source scripts/gen_build_combination.sh

nr=0
while IFS= read -r LINE; do
	nr=$((nr+1))

	check_project "$LINE"
	[ "$?" -ne 0 ] && continue
	source scripts/env.sh $LINE >> $BUILD_LOG 2>&1
	[ "$?" -ne 0 ] && echo "Ignore unsupported combination! $LINE" && continue
	make distclean
	[ "$?" -ne 0 ] && echo "Failed to make distclean! $LINE" && return 2
	echo -n -e "$nr. Building $LINE ...\t"
	make >> $BUILD_LOG 2>&1
	[ "$?" -ne 0 ] && echo "failed!" && cat $BUILD_LOG && touch $LAST_BUILD_FAILURE && echo -e "\nAborted with errors!\n" && return 3
	grep -qr "warning: " $BUILD_LOG
	[ "$?" -eq 0 ] && echo "with warnings!" && cat $BUILD_LOG && touch $LAST_BUILD_FAILURE && echo -e "\nAborted with warnings!\n" && return 1
	echo "OK"
	rm -f $LAST_BUILD_FAILURE
	if [[ "$SUBMIT_TYPE" == "daily" ]]; then
		if [[ "$ARCH" == "arm64" ]] && [[ "$PRODUCT" == "speaker" ]]; then
			make_image >> $BUILD_LOG 2>&1
		fi
		publish_images >> $BUILD_LOG 2>&1
		[ "$?" -ne 0 ] && echo "Failed to publish images!" && return 4
	fi
done <"$BUILD_COMBINATION"

[[ "$SUBMIT_TYPE" == "daily" ]] && post_publish_images >> $BUILD_LOG 2>&1

echo -e "======== Done ========\n" | tee -a $BUILD_LOG
