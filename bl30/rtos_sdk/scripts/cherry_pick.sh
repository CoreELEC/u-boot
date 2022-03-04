#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

if [ -n "$GIT_CHERRY_PICK" ]; then
	[ -z "$CURRENT_MANIFEST_FILE" ] && CURRENT_MANIFEST_FILE="manifest.xml"
	[ ! -f $CURRENT_MANIFEST_FILE ] && repo manifest -r -o $CURRENT_MANIFEST_FILE

	echo "$GIT_CHERRY_PICK" | while read line
	do
		pattern=":29418/"
		for keyword in $line; do
			if [[ $keyword == *$pattern* ]]; then
				GIT_PROJECT=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`
				break;
			fi
		done

		echo -e "\n======== Applying manual patch on Project $GIT_PROJECT ========"
		keyline=`grep "name=\"$GIT_PROJECT\"" $CURRENT_MANIFEST_FILE`

		for keyword in $keyline; do
			if [[ $keyword == path=* ]]; then
				repo_path=`echo ${keyword#*path=} | sed 's/\"//g'`
				break;
			fi
		done

		if [ -d "$repo_path" ]; then
			pushd $repo_path > /dev/null
			cmd=`echo $line | sed -e 's/ssh:\/\/.*@scgit.amlogic.com/ssh:\/\/scgit.amlogic.com/'`
			eval $cmd
			if [ "$?" -ne 0 ]; then
				echo -e "========= Applying patch failed! =========\n"
				exit 1
			fi
			popd > /dev/null
		else
			echo "No such directory! $repo_path"
			exit 1
		fi
        echo -e "======== Done ========\n"
	done
fi
