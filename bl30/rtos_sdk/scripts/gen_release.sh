#!/bin/bash
#
# Copyright (c) 2021-2024 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

docs_DIR=$PWD/arch/$ARCH/docs
DOC_PROJECT_NUMBER_LINE=`grep "PROJECT_NUMBER.*=" $docs_DIR/Doxyfile`
RELEASE_VERSION=`date +%y.%m.%d`
SDK_BASE=`basename $PWD`

if [ -f $PWD/CMakeLists.txt ] && [ -f $PWD/Kconfig ]; then
	[ -d $docs_DIR ] && sed -i "s/PROJECT_NUMBER.*=.*/PROJECT_NUMBER         = $RELEASE_VERSION/" $docs_DIR/Doxyfile
	(cd ..; \
	tar --exclude-vcs --exclude=.repo --exclude=cscope.* --exclude=output -cJf rtos_sdk_$RELEASE_VERSION.tar.xz $SDK_BASE; \
	mv rtos_sdk_$RELEASE_VERSION.tar.xz $SDK_BASE; \
	cd - > /dev/null)
	sed -i "s/PROJECT_NUMBER.*=.*/$DOC_PROJECT_NUMBER_LINE/" $docs_DIR/Doxyfile
else
	echo "Please execute make in advance!"
fi

