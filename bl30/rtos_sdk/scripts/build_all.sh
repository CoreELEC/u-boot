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

# Manually cherry pick patches
./scripts/cherry_pick.sh

source scripts/gen_build_combination.sh

i=0
while IFS= read -r LINE; do
	[[ "$i" -ne 0 ]] && echo ""
	i=$((i + 1))

	check_project "$LINE"
	[ "$?" -ne 0 ] && continue
	source scripts/env.sh $LINE
	[ "$?" -ne 0 ] && echo "Ignore unsupported combination!" && continue
	make distclean
	[ "$?" -ne 0 ] && echo "Failed to make distclean!" && exit 2
	make
	[ "$?" -ne 0 ] && echo "Failed to make!" && exit 3
	if [[ "$SUBMIT_TYPE" == "daily" ]]; then
		publish_image
		[ "$?" -ne 0 ] && echo "Failed to source scripts/scp.sh!" && exit 4
	fi
done <"$BUILD_COMBINATION"

echo "Build completed!"
