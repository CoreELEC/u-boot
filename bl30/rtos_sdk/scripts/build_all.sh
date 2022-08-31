#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

source scripts/publish.sh

if [[ "$SUBMIT_TYPE" == "daily" ]] || [[ "$SUBMIT_TYPE" == "release" ]]; then
	make docs
	if [ -d $LOCAL_DOC_PATH ]; then
		pushd $LOCAL_DOC_PATH >/dev/null
		publish_docoment
		if [ $? -ne 0 ]; then
			echo "Failed to update document"
		else
			echo "Document updated!"
		fi
		popd >/dev/null
	else
		echo "$LOCAL_DOC_PATH not exist!"
	fi
fi

source scripts/gen_build_combination.sh

[ -z "$BUILD_LOG" ] && BUILD_LOG="output/build.log"
# Clear build log
cat <<EOF > $BUILD_LOG
EOF

nr=0
while IFS= read -r LINE; do
	nr=$((nr+1))

	check_project "$LINE"
	[ "$?" -ne 0 ] && continue
	source scripts/env.sh $LINE >> $BUILD_LOG 2>&1
	[ "$?" -ne 0 ] && echo "Ignore unsupported combination! $LINE" && continue
	make distclean
	[ "$?" -ne 0 ] && echo "Failed to make distclean! $LINE" && exit 2
	echo -n "$nr. Building $LINE ... "
	make >> $BUILD_LOG 2>&1
	[ "$?" -ne 0 ] && echo "failed!" && exit 3
	echo "OK!"
	if [[ "$SUBMIT_TYPE" == "daily" ]]; then
		if [[ "$BOARD" == "ad403_a113l" ]] && [[ "$ARCH" == "arm64" ]] && [[ "$PRODUCT" == "speaker" ]]; then
			make_image >> $BUILD_LOG 2>&1
		fi
		publish_images >> $BUILD_LOG 2>&1
		[ "$?" -ne 0 ] && echo "Failed to publish images!" && exit 4
	fi
done <"$BUILD_COMBINATION"

echo "Build completed!"
