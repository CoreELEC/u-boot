#!/bin/bash

prj_dir=`pwd`

input="$prj_dir/.repo/manifests/default.xml"
cmake_file="$prj_dir/CMakeLists.txt"
kconfig_file="$prj_dir/Kconfig"
exclude_dir="products"

cat <<EOF > $cmake_file
enable_language(C CXX ASM)

target_include_directories(
	\${TARGET_NAME}
	PUBLIC
	include
)

EOF

cat <<EOF > $kconfig_file
EOF

while IFS= read -r line
do
	keyword=`echo "$line" | grep 'path=.* name=' | awk '{print $2}'`
	if [ $keyword ]; then
		repo_path=`echo ${keyword#*path=} | sed 's/\"//g'`
		category=`dirname $repo_path`
		# Generate root CMakeLists.txt
		if [ -f $repo_path/CMakeLists.txt ] && [ "$category" != "$exclude_dir" ]; then
			echo "add_subdirectory($repo_path)" >> $cmake_file
		fi

		# Generate root Kconfig
		if [ -f $repo_path/Kconfig ]; then
			if [ "$last_category" != "$category" ]; then
				if [ $last_category ]; then
					echo -e "endmenu\n" >> $kconfig_file
				fi
				echo "menu \"${category^} Options\"" >> $kconfig_file
			fi
			echo "source \"$repo_path/Kconfig\"" >> $kconfig_file
			last_category=$category
		fi
	fi
done < "$input"

echo "endmenu" >> $kconfig_file
