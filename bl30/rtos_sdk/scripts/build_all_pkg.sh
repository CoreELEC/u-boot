#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

source scripts/publish.sh

function get_new_package_dir() {
	filelist=$(ls -t output/packages)
	fileArry=($filelist)
	CURRENT_PRODUCTS_DIR_NAME=${fileArry[0]}
	export CURRENT_PRODUCTS_DIR_NAME
}

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

source scripts/gen_package_combination.sh

index=0
while IFS= read -r LINE; do
	source scripts/pkg_env.sh $index gen_all
	[ "$?" -ne 0 ] && echo "Ignore unsupported combination!" && continue
	make package
	get_new_package_dir
	[ "$?" -ne 0 ] && echo "Failed to make!" && exit 3
	if [[ "$SUBMIT_TYPE" == "release" ]]; then
		publish_packages
		[ "$?" -ne 0 ] && echo "Failed to publish packages!" && exit 4
	fi
	index=$((index + 1))
done <"$PACKAGE_COMBINATION"

echo "Build completed!"
