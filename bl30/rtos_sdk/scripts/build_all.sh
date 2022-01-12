#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

#usage:./scripts/build_all.sh at rtos sdk root dir

set -e

BUILD_COMBINATION="$PWD/scripts/build_combination.txt"

i=0
while IFS= read -r LINE; do
	[[ "$i" -ne 0 ]] && echo ""
	i=$((i+1))
	make distclean
	source scripts/env.sh $LINE
	make
done < "$BUILD_COMBINATION"
