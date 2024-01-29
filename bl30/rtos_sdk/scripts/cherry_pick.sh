#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

get_repo_path() {
	for keyword in $keyline; do
		if [[ $keyword == $pattern* ]]; then
			repo_path=`echo ${keyword#*${pattern}} | sed 's/\"//g'`
			break;
		fi
	done
}

cherry_pick() {
	if [ -d "$repo_path" ]; then
		pushd $repo_path > /dev/null
		echo -e "========toby debug (1):$GERRIT_SERVER (2):$GERRIT_PORT (3):$GERRIT_PROJECT (4):$GERRIT_REFSPEC ========"
		git fetch ssh://${GERRIT_SERVER}:${GERRIT_PORT}/${GERRIT_PROJECT} ${GERRIT_REFSPEC}
		git cherry-pick FETCH_HEAD
		if [ "$?" -ne 0 ]; then
			git status
			git log -1
			echo -e "\n${FUNCNAME[0]}: Failed to apply patch!\n"
			return 1
		fi
		popd > /dev/null
	else
		echo -e "\n${FUNCNAME[0]}: No such directory! $repo_path\n"
		return 1
	fi
}

apply_patch_by_change_number() {
	[ -z "$GERRIT_CHANGE_NUMBER" -o -z "$GERRIT_PROJECT" -o -z "$GERRIT_REFSPEC" ] && [ -z "$MANUAL_GERRIT_CHANGE_NUMBER" ] && return

	if [ -n "$GERRIT_CHANGE_NUMBER" ] && [ -n "$GERRIT_PROJECT" ] && [ -n "$GERRIT_REFSPEC" ]; then
		echo -e "======== Auto-applying Gerrit change $GERRIT_CHANGE_NUMBER on Project $GERRIT_PROJECT ========"
	elif [ -n "$MANUAL_GERRIT_CHANGE_NUMBER" ]; then
		ssh -p $GERRIT_PORT $GERRIT_SERVER gerrit query --format=JSON --current-patch-set status:open change:$MANUAL_GERRIT_CHANGE_NUMBER > $GERRIT_QUERY_RESULT
		GERRIT_PROJECT=$(jq -r '.project // empty' $GERRIT_QUERY_RESULT)
		GERRIT_REFSPEC=$(jq -r '.currentPatchSet.ref // empty' $GERRIT_QUERY_RESULT)
		echo -e "======== Manually applying Gerrit change $MANUAL_GERRIT_CHANGE_NUMBER on Project $GERRIT_PROJECT ========"
	fi

	keyline=`grep "name=\"$GERRIT_PROJECT\"" $MANIFEST`
	pattern="path="
	get_repo_path

	cherry_pick
	[ "$?" -ne 0 ] && return 1
	echo -e "======== Done ========\n"
}

apply_patch_by_gerrit_topic() {
	[ -z "$MANUAL_GERRIT_TOPIC" ] && return 0

	ssh -p $GERRIT_PORT $GERRIT_SERVER gerrit query --format=JSON --current-patch-set status:open topic:$MANUAL_GERRIT_TOPIC > $GERRIT_QUERY_RESULT
	GERRIT_PROJECTS=$(jq -r '.project // empty' $GERRIT_QUERY_RESULT)
	GERRIT_REFSPECS=$(jq -r '.currentPatchSet.ref // empty' $GERRIT_QUERY_RESULT)

	echo -e "======== Manually applying Gerrit Topic: $MANUAL_GERRIT_TOPIC ========"

	i=1
	for GERRIT_PROJECT in $GERRIT_PROJECTS; do
		echo "-------- Applying patch $i on Project $GERRIT_PROJECT --------"
		keyline=`grep "name=\"$GERRIT_PROJECT\"" $MANIFEST`
		pattern="path="
		get_repo_path

		GERRIT_REFSPEC=$(echo $GERRIT_REFSPECS | awk "{print \$$i}")
		cherry_pick
		[ "$?" -ne 0 ] && return 1
		echo -e "-------- Done --------\n"
		i=$((i+1))
	done

	i=$((i-1))
	[[ "$i" -eq 1 ]] && echo -e "======== Applied $i patch for $MANUAL_GERRIT_TOPIC ========\n"
	[[ "$i" -gt 1 ]] && echo -e "======== Applied $i patches for $MANUAL_GERRIT_TOPIC ========\n"

	return 0
}

apply_patch_by_gerrit_url() {
	[ -z "$GIT_CHERRY_PICK" ] && return

	while IFS= read -r line
	do
		pattern=":29418/"
		for keyword in $line; do
			if [[ $keyword == *$pattern* ]]; then
				GIT_PROJECT=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`
				break;
			fi
		done

		echo -e "\n-------- Manually applying patch on Project $GIT_PROJECT --------"
		keyline=`grep "name=\"$GIT_PROJECT\"" $MANIFEST`
		pattern="path="
		get_repo_path

		if [ -d "$repo_path" ]; then
			pushd $repo_path > /dev/null
			cmd=`echo $line | sed -e 's/ssh:\/\/.*@scgit.amlogic.com/ssh:\/\/scgit.amlogic.com/'`
			eval $cmd
			if [ "$?" -ne 0 ]; then
				git status
				git log -1
				echo -e "\n${FUNCNAME[0]}: Failed to apply patch!\n"
				exit 1
			fi
			popd > /dev/null
		else
			echo -e "\n${FUNCNAME[0]}: No such directory! $repo_path\n"
			exit 1
		fi
		echo -e "-------- Done --------\n"
	done <<< "$GIT_CHERRY_PICK"
}

[ -z "$OUTPUT_DIR" ] && OUTPUT_DIR=$PWD/output
[ ! -d $OUTPUT_DIR ] && mkdir -p $OUTPUT_DIR

[ -z "$MANIFEST" ] && MANIFEST="$OUTPUT_DIR/manifest.xml"
[ ! -f $MANIFEST ] && repo manifest -r -o $MANIFEST

[ -z "$GERRIT_SERVER" ] && GERRIT_SERVER="scgit.amlogic.com"
[ -z "$GERRIT_PORT" ] && GERRIT_PORT="29418"
[ -z "$GERRIT_QUERY_RESULT" ] && GERRIT_QUERY_RESULT="$OUTPUT_DIR/topic_changes.txt"

apply_patch_by_change_number
apply_patch_by_gerrit_topic
