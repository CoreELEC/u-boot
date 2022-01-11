#!/bin/bash

# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

###############################################################
# Function: Auto-generate root CMakeLists.txt and Kconfig according to manifest.xml.
###############################################################

cmake_file="$PWD/CMakeLists.txt"
kconfig_file="$PWD/Kconfig"
build_dir="build"
exclude_dir="products docs"
special_dirs="arch soc boards"

RTOS_SDK_MANIFEST_FILE="$kernel_BUILD_DIR/rtos_sdk_manifest.xml"
STAMP="$kernel_BUILD_DIR/.stamp"

# Check whether the project is a repo
repo manifest >/dev/null 2>&1
[ "$?" -ne 0 ] && exit 0

if [ -s $RTOS_SDK_MANIFEST_FILE ] && [ -s $kconfig_file ] && [ $PWD/Makefile -ot $kconfig_file ] && [ $kconfig_file -ot $STAMP ]; then
	exit 0
fi

# Generate manifest.xml
repo manifest > $RTOS_SDK_MANIFEST_FILE
if [ ! -f $RTOS_SDK_MANIFEST_FILE ]; then
	echo "Faild to save $RTOS_SDK_MANIFEST_FILE"
	exit 1
fi

if [[ "$PRODUCT" == aocpu ]]; then
	sed -i '/path="drivers"/d' $RTOS_SDK_MANIFEST_FILE
else
	sed -i '/path="drivers_aocpu"/d' $RTOS_SDK_MANIFEST_FILE
fi

# Write the fixed content to CMakeLists.txt
cat <<EOF > $cmake_file
enable_language(C CXX ASM)

EOF

# Clear Kconfig
cat <<EOF > $kconfig_file
EOF

# filter manifest.xml of RTOS SDK
sed -i '/rtos_sdk\//!d' $RTOS_SDK_MANIFEST_FILE
# figure out the $relative_dir and its column
pattern="path="
i=0
keyline=`grep 'path=".*build"' $RTOS_SDK_MANIFEST_FILE`
for keyword in $keyline; do
	let i++
	if [[ $keyword == $pattern* ]]; then
		repo_path=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`
		relative_dir=`dirname $repo_path`
		break;
	fi
done

if [[ $relative_dir == . ]]; then
	pattern="path="
else
	pattern="path=\"${relative_dir}/"
fi

# sort manifest.xml of RTOS SDK
sort -k $i $RTOS_SDK_MANIFEST_FILE -o $RTOS_SDK_MANIFEST_FILE

while IFS= read -r line
do
	keyline=`echo "$line" | grep 'name=.* path='`
	for keyword in $keyline; do
		if [[ $keyword == path=* ]]; then
			repo_path=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`

			if [[ $repo_path == drivers* ]] || [[ $repo_path == third_party* ]]; then
				category=`basename $repo_path | sed 's/_/ /g'`
				category=`echo $category | sed "s/ $PRODUCT//g"`
			else
				category=`dirname $repo_path`
			fi

			# exclude other ARCH dirs
			if [[ $repo_path == docs* ]] || [[ $repo_path == products* ]]; then
				continue
			fi

			# substitute ARCH dirs with viarable
			case $special_dirs in
				*"$category"*) arch=`basename $repo_path`
					       if [ "$arch" == "$ARCH" ]; then
							cmake_path="$category/\${ARCH}"
							kconfig_path="$category/\$(ARCH)"
					       else
							continue
					       fi;;
				* ) cmake_path=$repo_path
				    kconfig_path=$repo_path;;
			esac

			# Generate root CMakeLists.txt
			if [ -f $repo_path/CMakeLists.txt ]; then
				echo "add_subdirectory($cmake_path)" >> $cmake_file
			fi

			# Generate root Kconfig
			if [ -f $repo_path/Kconfig ]; then
				if [ "$last_category" != "$category" ]; then
					if [ -n "$last_category" ]; then
						echo -e "endmenu\n" >> $kconfig_file
					fi

					if [ "$category" == "wcn" ]; then
						echo "menu \"${category^^} Options\"" >> $kconfig_file
					else
						echo "menu \"${category^} Options\"" >> $kconfig_file
					fi
				fi

				echo "source \"$kconfig_path/Kconfig\"" >> $kconfig_file
				last_category=$category
			fi
			break;
		fi
	done
done < "$RTOS_SDK_MANIFEST_FILE"

echo "endmenu" >> $kconfig_file

sleep 1
touch $STAMP
