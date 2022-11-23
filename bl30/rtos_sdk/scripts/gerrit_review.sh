#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

# $1: Review result
gerrit_review_for_gerrit_topic() {
	[ -z "$MANUAL_GERRIT_TOPIC" ] && return

	[ -z "$OUTPUT_DIR" ] && OUTPUT_DIR=$PWD/output
	[ -z "$GERRIT_SERVER" ] && GERRIT_SERVER="scgit.amlogic.com"
	[ -z "$GERRIT_PORT" ] && GERRIT_PORT="29418"
	[ -z "$GERRIT_QUERY_RESULT" ] && GERRIT_QUERY_RESULT="$OUTPUT_DIR/topic_changes.txt"

	[ ! -f "$GERRIT_QUERY_RESULT" ] && echo "${FUNCNAME[0]}: No such file! $GERRIT_QUERY_RESULT" && exit 1
	[ $# -ne 1 ] && echo "${FUNCNAME[0]}: Invalid parameters! $*" && exit 1

	if [ "$1" = "Start" ]; then
		verify_param=""
	elif [ "$1" = "SUCCESS" ] || [ "$1" = "FAIL" ]; then
		[ "$1" = "SUCCESS" ] && verify_score="+1"
		[ "$1" = "FAIL" ] && verify_score="-1"
		verify_param="--verified $verify_score"
		echo -e "======== Verifying Gerrit Topic: $MANUAL_GERRIT_TOPIC ========"
	else
		echo "${FUNCNAME[0]}: Invalid parameter $1" && exit 1
	fi

	review_msg="Build ${BUILD_URL}: $1"

	GERRIT_CHANGE_NUMBERS=$(jq -r '.number // empty' $GERRIT_QUERY_RESULT)
	GERRIT_PATCHSET_NUMBERS=$(jq -r '.currentPatchSet.number // empty' $GERRIT_QUERY_RESULT)

	i=1
	for GERRIT_CHANGE_NUMBER in $GERRIT_CHANGE_NUMBERS; do
		GERRIT_PATCHSET_NUMBER=$(echo $GERRIT_PATCHSET_NUMBERS | awk "{print \$$i}")
		[ "$1" != "Start" ] && echo -n -e "$GERRIT_CHANGE_NUMBER/$GERRIT_PATCHSET_NUMBER $verify_score ...\t"
		ssh -p $GERRIT_PORT $GERRIT_SERVER gerrit review "${verify_param}" -m "'${review_msg}'" $GERRIT_CHANGE_NUMBER,$GERRIT_PATCHSET_NUMBER
		if [ "$1" != "Start" ]; then
			if [ "$?" -eq 0 ]; then
				echo "done"
			else
				echo "failed"
			fi
		fi
		i=$((i+1))
	done

	if [ "$1" != "Start" ]; then
		i=$((i-1))
		[ "$i" -eq 1 ] && echo -e "======== Verified $i Gerrit change for $MANUAL_GERRIT_TOPIC ========\n"
		[ "$i" -gt 1 ] && echo -e "======== Verified $i Gerrit changes for $MANUAL_GERRIT_TOPIC ========\n"
	fi

	[ "$1" = "FAIL" ] && echo "FAIL" && exit 1
	echo "SUCCESS"
	return 0
}
