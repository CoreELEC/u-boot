#!/bin/bash
#usage:./scripts/auto_build_test.sh at rtos sdk root dir

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
