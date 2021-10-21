#!/bin/bash

prj_dir=`pwd`

input="$prj_dir/.repo/manifests/default.xml"
cmake_file="$prj_dir/CMakeLists.txt"
kconfig_file="$prj_dir/Kconfig"
exclude_dir="products"
special_dirs="arch soc boards"
drivers_dir="drivers"

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
		if [[ $repo_path == $drivers_dir* ]] ; then
			category=$repo_path
		else
			category=`dirname $repo_path`
		fi

		if [[ $repo_path == $exclude_dir/* ]] ; then
			continue
		fi

		# exclude other ARCH dirs
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
				if [ $last_category ]; then
					echo -e "endmenu\n" >> $kconfig_file
				fi
				echo "menu \"${category^} Options\"" >> $kconfig_file
			fi

			echo "source \"$kconfig_path/Kconfig\"" >> $kconfig_file
			last_category=$category
		fi
	fi
done < "$input"

echo "endmenu" >> $kconfig_file
