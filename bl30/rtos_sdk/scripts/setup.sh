#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

###############################################################
# Function: Auto-generate root CMakeLists.txt and Kconfig according to manifest.xml.
###############################################################

cmake_file="$PWD/CMakeLists.txt"
kconfig_file="$PWD/Kconfig"
build_dir="build_system"
exclude_dirs="boot products docs"
special_dirs="arch soc boards"
toolchain_dir="arch/$ARCH/toolchain"

DEFAULT_RTOS_SDK_MANIFEST="$PWD/products/$PRODUCT/rtos_sdk_manifest.xml"
RTOS_SDK_MANIFEST_FILE="$kernel_BUILD_DIR/rtos_sdk_manifest.xml"
RTOS_SDK_MANIFEST_OLD_FILE="$kernel_BUILD_DIR/rtos_sdk_manifest_old.xml"
STAMP="$kernel_BUILD_DIR/.stamp"

[ -n "$1" ] && BUILD_DIR=$1;
RTOS_SDK_VERSION_FILE="$BUILD_DIR/sdk_ver.h"

#COMPILE_TIME="$(shell date +%g.%V.%u" "%H:%M:%S)"
COMPILE_TIME=`date +%F" "%T`

echo "#define CONFIG_BOARD_NAME \"$BOARD\"" > $RTOS_SDK_VERSION_FILE
echo "#define CONFIG_PRODUCT_NAME \"$PRODUCT\"" >> $RTOS_SDK_VERSION_FILE
echo "#define CONFIG_COMPILE_TIME \"$COMPILE_TIME\"" >> $RTOS_SDK_VERSION_FILE

# Check whether the project is a valid repo
repo manifest 2>&1 | grep -q $build_dir
if [ "$?" -ne 0 ]; then
	echo "Non-repo source code"
	if [ -f $DEFAULT_RTOS_SDK_MANIFEST ]; then
		echo "Use default manifest: $DEFAULT_RTOS_SDK_MANIFEST"
		cp -f $DEFAULT_RTOS_SDK_MANIFEST $RTOS_SDK_MANIFEST_FILE
	else
		echo "Default manifest.xml not found!"
		exit 0
	fi
else
	echo "Generate manifest.xml"
	repo manifest > $RTOS_SDK_MANIFEST_FILE
	if [ ! -f $RTOS_SDK_MANIFEST_FILE ]; then
		echo "Faild to save $RTOS_SDK_MANIFEST_FILE"
		exit 1
	fi
fi

# Get SDK_VERSION
pattern="revision="
keyline=`grep 'default .* revision' $RTOS_SDK_MANIFEST_FILE`
for keyword in $keyline; do
	let i++
	if [[ $keyword == $pattern* ]]; then
		SDK_VERSION=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`
		break;
	fi
done
echo "#define CONFIG_VERSION_STRING \"$SDK_VERSION\"" >> $RTOS_SDK_VERSION_FILE

if [ -s $RTOS_SDK_MANIFEST_OLD_FILE ] && [ -s $kconfig_file ] && [ $kconfig_file -ot $STAMP ]; then
	is_update=`comm -3 <(sort $RTOS_SDK_MANIFEST_FILE) <(sort $RTOS_SDK_MANIFEST_OLD_FILE)`
	if [ -z "$is_update" ]; then
		exit 0
	else
		echo "Update top Kconfig and CMakelists.txt."
	fi
fi

# Back up manifest.xml
cp -arf $RTOS_SDK_MANIFEST_FILE $RTOS_SDK_MANIFEST_OLD_FILE

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

# Write the toolchain options to Kconfig
if [ -f $toolchain_dir/Kconfig ]; then
	category=`basename $toolchain_dir`
	toolchain_kconfig_path="arch/\${ARCH}/toolchain"
	echo "menu \"${category^} Options\"" > $kconfig_file
	echo "source \"$toolchain_kconfig_path/Kconfig\"" >> $kconfig_file
	echo -e "endmenu\n" >> $kconfig_file
fi

# Figure out the $relative_dir and its column
[ -z "$REPO_DIR" ] && REPO_DIR=$PWD
pattern="path="
i=0
keyline=`grep "path=\".*$build_dir\"" $RTOS_SDK_MANIFEST_FILE`
for keyword in $keyline; do
	let i++
	if [[ $keyword == $pattern* ]]; then
		repo_path=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`
		relative_dir=`dirname $repo_path`
		# Filter current project
		if [[ $REPO_DIR == *$relative_dir ]]; then
			break
		fi
	fi
done

if [[ $relative_dir == . ]]; then
	pattern="path="
else
	pattern="path=\"${relative_dir}/"
fi

# Sort manifest.xml of RTOS SDK
sort -k $i $RTOS_SDK_MANIFEST_FILE -o $RTOS_SDK_MANIFEST_FILE

while IFS= read -r line
do
	keyline=`echo "$line" | grep "$pattern"`
	[ -z "$keyline" ] && continue

	for keyword in $keyline; do
		if [[ $keyword == path=* ]]; then
			repo_path=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`

			if [[ $repo_path == drivers* ]] || [[ $repo_path == benchmark* ]]; then
				category=`basename $repo_path | sed 's/_/ /g'`
				category=`echo $category | sed "s/ $PRODUCT//g"`
			else
				category=`dirname $repo_path`
			fi

			# exclude some dirs
			skip_flag=0
			for exclude_dir in $exclude_dirs; do
				if [[ $repo_path == $exclude_dir* ]]; then
					skip_flag=1
					break
				fi
			done
			[ "$skip_flag" -eq 1 ] && continue

			# substitute ARCH dirs with viarable
			case $special_dirs in
				*"$category"*) arch=`basename $repo_path`
					       if [ "$arch" == "$ARCH" ]; then
							cmake_path="$category/\${ARCH}"
							kconfig_path="$category/\${ARCH}"
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

					if [ "$category" == "wcn" -o "$category" == "soc" ]; then
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
