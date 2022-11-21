#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

gerrit_review_for_gerrit_topic() {
	[ -z "$OUTPUT_DIR" ] && OUTPUT_DIR=$PWD/output
	[ -z "$GERRIT_SERVER" ] && GERRIT_SERVER="scgit.amlogic.com"
	[ -z "$GERRIT_PORT" ] && GERRIT_PORT="29418"
	[ -z "$GERRIT_QUERY_RESULT" ] && GERRIT_QUERY_RESULT="$OUTPUT_DIR/topic_changes.txt"

	if [ -z "$MANUAL_GERRIT_TOPIC" ] || [ ! -f "$GERRIT_QUERY_RESULT" ]; then
		return
	fi

	if [ "$1" = "SUCCESS" ]; then
		verify_score="+1"
	elif [ "$1" = "FAIL" ]; then
		verify_score="-1"
	else
		echo "gerrit_review_for_gerrit_topic: Invalid parameter $1"
		return
	fi

	review_msg="Build ${BUILD_URL}: $1"

	GERRIT_CHANGE_NUMBERS=$(jq -r '.number // empty' $GERRIT_QUERY_RESULT)
	GERRIT_PATCHSET_NUMBERS=$(jq -r '.currentPatchSet.number // empty' $GERRIT_QUERY_RESULT)

	i=1
	for GERRIT_CHANGE_NUMBER in $GERRIT_CHANGE_NUMBERS; do
		GERRIT_PATCHSET_NUMBER=$(echo $GERRIT_PATCHSET_NUMBERS | awk "{print \$$i}")
		echo -n "$GERRIT_CHANGE_NUMBER/$GERRIT_PATCHSET_NUMBER $verify_score ... "
		ssh -p $GERRIT_PORT $GERRIT_SERVER gerrit review --verified "${verify_score}" -m "'${review_msg}'" $GERRIT_CHANGE_NUMBER,$GERRIT_PATCHSET_NUMBER
		if [ "$?" -eq 0 ]; then
			echo "OK"
		else
			echo "failed"
		fi
		i=$((i+1))
	done

	if [ "$1" = "FAIL" ]; then
		exit 1
	fi
}
