#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

DIFF_MANIFEST="$OUTPUT_DIR/diff_manifest.xml"
JENKINS_TRIGGER="$OUTPUT_DIR/jenkins_trigger.txt"

gen_jenkins_trigger() {
	if [ -s $CURRENT_MANIFEST ]; then
		echo -e "======== Generate Jenkins Trigger ========\n"

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
	fi
	rm -f $LAST_MANIFEST $CURRENT_MANIFEST $DIFF_MANIFEST
}

if [ ! -f $LAST_MANIFEST ] && [ -f $CURRENT_MANIFEST ]; then
	gen_jenkins_trigger
fi
if [ -f $LAST_MANIFEST ] && [ -f $CURRENT_MANIFEST ]; then
	comm -3 <(sort $LAST_MANIFEST) <(sort $CURRENT_MANIFEST) > $DIFF_MANIFEST
	[ -s $DIFF_MANIFEST ] && gen_jenkins_trigger
fi
