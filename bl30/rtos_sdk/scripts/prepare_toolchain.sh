#!/bin/bash
#
# Copyright (c) 2021-2024 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

[ -z "$OUTPUT_DIR" ] && OUTPUT_DIR=$PWD/output
[ -z "$OUTPUT_DIR" ] && TOOLCHAIN_DIR=$OUTPUT_DIR/toolchains/$COMPILER-$TOOLCHAIN_KEYWORD
CROSSTOOL=$PWD/arch/$ARCH/toolchain/$COMPILER*$TOOLCHAIN_KEYWORD

if [ -n "$COMPILER" ]; then
	if [[ "$ARCH" == "arm" || "$ARCH" == "arm64" ]]; then
		if [ ! -d $TOOLCHAIN_DIR ]; then
			echo "Extracting cross toolchain ..."
			mkdir -p $TOOLCHAIN_DIR
			tar -xf $CROSSTOOL.tar.xz -C $TOOLCHAIN_DIR --strip-components=1
			touch $TOOLCHAIN_DIR
		fi
		if ( find $CROSSTOOL.tar.xz -newer $TOOLCHAIN_DIR | grep -q $CROSSTOOL.tar.xz ); then
			echo "Updating cross toolchain ..."
			rm -rf $TOOLCHAIN_DIR/*
			tar -xf $CROSSTOOL.tar.xz -C $TOOLCHAIN_DIR --strip-components=1
			touch $TOOLCHAIN_DIR
		fi
		if [ -f $CROSSTOOL.patch ]; then
			cd $OUTPUT_DIR/toolchains
			if patch -N -f -s --dry-run -p0 < $CROSSTOOL.patch >/dev/null; then
				echo "Preparing cross toolchain ..."
				patch -s -p0 < $CROSSTOOL.patch
			fi
		fi
	fi
else
	echo "COMPILER is not set, Please execute source scripts/env.sh"
	exit 1
fi
