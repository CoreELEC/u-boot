#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

[ -z "$CURRENT_MANIFEST" ] && CURRENT_MANIFEST="curr_manifest.xml"
[ ! -f $CURRENT_MANIFEST ] && repo manifest -r -o $CURRENT_MANIFEST

if [ -n "$GIT_CHERRY_PICK" ]; then
	while IFS= read -r line
	do
		pattern=":29418/"
		for keyword in $line; do
			if [[ $keyword == *$pattern* ]]; then
				GIT_PROJECT=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`
				break;
			fi
		done

		echo -e "\n-------- Applying manual patch on Project $GIT_PROJECT --------"
		keyline=`grep "name=\"$GIT_PROJECT\"" $CURRENT_MANIFEST`

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
				git status
				git log -1
				echo -e "-------- Failed to apply patch! --------"
				exit 1
			fi
			popd > /dev/null
		else
			echo "No such directory! $repo_path"
			exit 1
		fi
		echo -e "-------- Done --------\n"
	done <<< "$GIT_CHERRY_PICK"
fi

if [ -n "$MANUAL_GERRIT_CHANGE_NUMBER" ]; then
	GERRIT_SERVER="scgit.amlogic.com"
	GERRIT_PORT="29418"
	GERRIT_QUERY_RESULT="changes.txt"
	ssh -p $GERRIT_PORT $GERRIT_SERVER gerrit query --format=JSON --current-patch-set status:open change:$GERRIT_CHANGE_NUMBER > $GERRIT_QUERY_RESULT
	GERRIT_PROJECT=$(jq -r '.project // empty' $GERRIT_QUERY_RESULT)
	GERRIT_CHANGE_REF=$(jq -r '.currentPatchSet.ref // empty' $GERRIT_QUERY_RESULT)

	echo -e "\n-------- Applying manual patch on Project $GERRIT_PROJECT --------"
	keyline=`grep "name=\"$GERRIT_PROJECT\"" $CURRENT_MANIFEST`

	for keyword in $keyline; do
		if [[ $keyword == path=* ]]; then
			repo_path=`echo ${keyword#*path=} | sed 's/\"//g'`
			break;
		fi
	done

	if [ -d "$repo_path" ]; then
		pushd $repo_path > /dev/null
		git fetch ssh://${GERRIT_SERVER}:${GERRIT_PORT}/${GERRIT_PROJECT} ${GERRIT_CHANGE_REF}
		git cherry-pick FETCH_HEAD
		if [ "$?" -ne 0 ]; then
			echo -e "-------- Applying patch failed! --------\n"
			exit 1
		fi
		popd > /dev/null
	else
		echo "No such directory! $repo_path"
		exit 1
	fi
	echo -e "-------- Done --------\n"
fi

if [ -n "$MANUAL_GERRIT_TOPIC" ]; then
	GERRIT_SERVER="scgit.amlogic.com"
	GERRIT_PORT="29418"
	GERRIT_QUERY_RESULT="changes.txt"
	ssh -p $GERRIT_PORT $GERRIT_SERVER gerrit query --format=JSON --current-patch-set status:open topic:$MANUAL_GERRIT_TOPIC > $GERRIT_QUERY_RESULT
	GERRIT_PROJECTS=$(jq -r '.project // empty' $GERRIT_QUERY_RESULT)
	GERRIT_CHANGE_REFS=$(jq -r '.currentPatchSet.ref // empty' $GERRIT_QUERY_RESULT)

	echo -e "\n======== Applying manual changes ========"

	i=1
	for GERRIT_PROJECT in $GERRIT_PROJECTS; do
		echo -e "\n-------- Applying manual patch $i on Project $GERRIT_PROJECT --------"
		keyline=`grep "name=\"$GERRIT_PROJECT\"" $CURRENT_MANIFEST`

		for keyword in $keyline; do
			if [[ $keyword == path=* ]]; then
				repo_path=`echo ${keyword#*path=} | sed 's/\"//g'`
				break;
			fi
		done

		if [ -d "$repo_path" ]; then
			GERRIT_CHANGE_REF=$(echo $GERRIT_CHANGE_REFS | awk "{print \$$i}")
			pushd $repo_path > /dev/null
			git fetch ssh://${GERRIT_SERVER}:${GERRIT_PORT}/${GERRIT_PROJECT} ${GERRIT_CHANGE_REF}
			git cherry-pick FETCH_HEAD
			if [ "$?" -ne 0 ]; then
				git status
				git log -1
				echo -e "-------- Failed to apply patch! --------"
				exit 1
			fi
			popd > /dev/null
		else
			echo "No such directory! $repo_path"
			exit 1
		fi
		echo -e "-------- Done --------"
		i=$((i+1))
	done

	i=$((i-1))
	[[ "$i" -eq 1 ]] && echo -e "\n======== Applied $i patch for $MANUAL_GERRIT_TOPIC ========\n"
	[[ "$i" -gt 1 ]] && echo -e "\n======== Applied $i patches for $MANUAL_GERRIT_TOPIC ========\n"
fi
