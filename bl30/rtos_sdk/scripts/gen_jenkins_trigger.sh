#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

gen_jenkins_trigger() {
	if [ -s $CURRENT_MANIFEST ]; then
		echo "======== Generate Jenkins Trigger ========"

		JENKINS_TRIGGER="$OUTPUT_DIR/jenkins_trigger.txt"
		rm -f $JENKINS_TRIGGER

		pattern="name="
		while IFS= read -r line
		do
			keyline=`echo "$line" | grep 'name=.* path='`
			unset repo_name
			for keyword in $keyline; do
				[[ $keyword == $pattern* ]] && repo_name=`echo ${keyword#*${pattern}} | sed 's/\"//g'`
			done

			if [ -n "$repo_name" ]; then
				echo "p=$repo_name" >> $JENKINS_TRIGGER
				echo "b=projects/amlogic-dev" >> $JENKINS_TRIGGER
			fi
		done < $CURRENT_MANIFEST

		echo -e "======== Done ========\n"
	fi
}

[ -z "$OUTPUT_DIR" ] && OUTPUT_DIR=$PWD/output
[ ! -d $OUTPUT_DIR ] && mkdir -p $OUTPUT_DIR

[ -z "$CURRENT_MANIFEST" ] && CURRENT_MANIFEST="$OUTPUT_DIR/curr_manifest.xml"
[ ! -f $CURRENT_MANIFEST ] && repo manifest -r -o $CURRENT_MANIFEST

[ -z "$LAST_MANIFEST" ] && LAST_MANIFEST="$OUTPUT_DIR/last_manifest.xml"

[ -z "$DIFF_MANIFEST" ] && DIFF_MANIFEST="$OUTPUT_DIR/diff_manifest.xml"

if [ ! -f $LAST_MANIFEST ]; then
	gen_jenkins_trigger
else
	comm -3 <(sort $LAST_MANIFEST) <(sort $CURRENT_MANIFEST) > $DIFF_MANIFEST
	[ -s $DIFF_MANIFEST ] && gen_jenkins_trigger
fi
