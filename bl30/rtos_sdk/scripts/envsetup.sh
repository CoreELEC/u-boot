#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

function aml_help() {
cat <<EOF

- aml_root:       Changes directory to the top of the tree, or a subdirectory.

- aml_reset:      Reset all repositories to remote branch among rtos sdk.
- aml_checkout:   Checkout trunk branch if track needed among rtos sdk.
- aml_git:        Execute git command for all repositories among rtos sdk.
- aml_pull:       Pull rebase all repositories among rtos sdk.

- aml_cgrep:      Greps on all local project C/C++ files.
- aml_kgrep:      Greps on all local project Kconfig files.
- aml_cmgrep:     Greps on all local project CMakelists.txt files.
- aml_shgrep:     Greps on all local project shell files.

EOF
}

function aml_top
{
    local TOPFILE=build_system/cmake/root.cmake
    if [ -n "$TOP" -a -f "$TOP/$TOPFILE" ] ; then
        (cd $TOP; PWD= /bin/pwd)
    else
        if [ -f $TOPFILE ] ; then
            PWD= /bin/pwd
        else
            local HERE=$PWD
            local T=
            while [ \( ! \( -f $TOPFILE \) \) -a \( $PWD != "/" \) ]; do
                \cd ..
                T=`PWD= /bin/pwd -P`
            done
            \cd $HERE
            if [ -f "$T/$TOPFILE" ]; then
                echo $T
            fi
        fi
    fi
}

function aml_trunk_remote
{
	trunk_remote_name=origin

	echo $trunk_remote_name
}

function aml_trunk_branch
{
	trunk_branch_name=projects/amlogic-dev

	echo $trunk_branch_name
}

function aml_trunk_name
{
	trunk_name=$(aml_trunk_remote)/$(aml_trunk_branch)

	echo $trunk_name
}

function aml_root()
{
    local T=$(aml_top)
    if [ "$T" ]; then
        if [ "$1" ]; then
            \cd $(gettop)/$1
        else
            \cd $(gettop)
        fi
    else
        echo "Locate the top dir failed."
    fi
}

function aml_reset()
{
	git_list_dir=$(find  $(aml_top) ! -path "*/.repo/*" -name .git | xargs dirname)

	for PRODUCT_PATH in $git_list_dir;
	do
		if [ -d $PRODUCT_PATH ];then
			echo product:${PRODUCT_PATH#*"$(aml_top)/"}
			git -C $PRODUCT_PATH reset -q --hard $(aml_trunk_name)
		fi
	done
}

function aml_checkout()
{
	git_list_dir=$(find  $(aml_top) ! -path "*/.repo/*" -name .git | xargs dirname)

	for PRODUCT_PATH in $git_list_dir;
	do
		if [ -d $PRODUCT_PATH ];then
			echo product:${PRODUCT_PATH#*"$(aml_top)/"}
			git -C $PRODUCT_PATH checkout $(aml_trunk_branch)
		 	if [ "$?" != 0 ]; then
				git -C $PRODUCT_PATH checkout --track $(aml_trunk_name)
		 	fi
		fi
	done
}

function aml_git()
{
	git_list_dir=$(find  $(aml_top) ! -path "*/.repo/*" -name .git | xargs dirname)

	for PRODUCT_PATH in $git_list_dir;
	do
		if [ -d $PRODUCT_PATH ];then
			echo product:${PRODUCT_PATH#*"$(aml_top)/"}
			git -C $PRODUCT_PATH $@
		fi
	done
}

function aml_pull()
{
	aml_checkout
	aml_git pull --rebase
}

function aml_cgrep()
{
    find $(aml_top) -name .repo -prune -o -name .git -prune -o -name output -prune -o -type f \
    	\( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
        -exec grep --color -n "$@" {} +
}

function aml_kgrep()
{
    find $(aml_top) -name .repo -prune -o -name .git -prune -o -name output -prune -o -type f \
    	\( -name 'Kconfig' \) -exec grep --color -n "$@" {} +
}

function aml_cmgrep()
{
    find $(aml_top) -name .repo -prune -o -name .git -prune -o -name output -prune -o -type f \
    	\( -name 'CMakeLists.txt' \) -exec grep --color -n "$@" {} +
}

function aml_shgrep()
{
    find $(aml_top) -name .repo -prune -o -name .git -prune -o -name output -prune -o -type f \
    	\( -name '*.sh' \) -exec grep --color -n "$@" {} +
}

