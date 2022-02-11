#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

# usage:./scripts/build_all.sh at rtos sdk root dir

BUILD_COMBINATION="$PWD/scripts/build_combination.txt"

# Build and upload document
if [[ "$SUBMIT_TYPE" == "daily" ]]; then
	make docs
	cd output/docs/html; find -type f -exec curl --ftp-create-dirs -T {} ftp://platform:platform@10.68.11.163:2222/Documents/Ecosystem/RTOS/rtos-sdk/{} \;; cd -
fi

# Build all projects
i=0
while IFS= read -r LINE; do
	[[ "$i" -ne 0 ]] && echo ""
	i=$((i+1))
	source scripts/env.sh $LINE
	[ "$?" -ne 0 ] && echo "Failed to source scripts/env.sh!" && exit 1
	make distclean
	[ "$?" -ne 0 ] && echo "Failed to make distclean!" && exit 2
	make
	[ "$?" -ne 0 ] && echo "Failed to make!" && exit 3
	if [[ "$SUBMIT_TYPE" == "daily" ]]; then
		source scripts/scp.sh
		[ "$?" -ne 0 ] && echo "Failed to source scripts/scp.sh!" && exit 4
	fi
done < "$BUILD_COMBINATION"
