#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#usage:./scripts/build_all.sh at rtos sdk root dir

set -e

BUILD_COMBINATION="$PWD/scripts/build_combination.txt"

# Build all projects
i=0
while IFS= read -r LINE; do
	[[ "$i" -ne 0 ]] && echo ""
	i=$((i+1))
	source scripts/env.sh $LINE
	make distclean
	make
	if [[ "$SUBMIT_TYPE" == "daily" ]]; then
		source scripts/scp.sh
	fi
done < "$BUILD_COMBINATION"

# Build and upload document
if [[ "$SUBMIT_TYPE" == "daily" ]]; then
	make docs
	cd output/docs/html; find -type f -exec curl --ftp-create-dirs -T {} ftp://platform:platform@10.68.11.163:2222/Documents/Ecosystem/RTOS/rtos-sdk/{} \;; cd -
fi
