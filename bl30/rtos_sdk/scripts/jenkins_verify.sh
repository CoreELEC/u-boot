#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

source /etc/profile.d/TOOLSENV.sh

export PATH=/opt/cmake-3.18.4-Linux-x86_64/bin/:$PATH
export PATH=/proj/coverity/cov-analysis/bin/:$PATH
export LM_LICENSE_FILE=/mnt/fileroot/jenkins/Xplorer_key.txt:$LM_LICENSE_FILE
export PATH=/opt/xtensa/XtDevTools/install/tools/RG-2018.9-linux/XtensaTools/bin:$PATH
export XTENSA_SYSTEM=/opt/xtensa/XtDevTools/install/builds/RG-2018.9-linux/Amlogic_v0/config
export XTENSA_CORE=Amlogic_v0

if [ -z "$MANIFEST_URL" ] || [ -z "$MANIFEST_BRANCH" ] || [ -z "$PROJECT_NAME" ] || [ -z "$BRANCH_NAME" ]; then
	echo "NULL params!"
	exit 1
fi

if [ "$SUBMIT_TYPE" = "daily" -o "$SUBMIT_TYPE" = "release" ];then
	BUILDCHECK_BASE_PATH=/mnt/fileroot/autobuild/workdir/workspace/RTOS/RTOS_SDK/patchbuild
elif [ "$SUBMIT_TYPE" = "every" ];then
	BUILDCHECK_BASE_PATH=/mnt/fileroot/jenkins/build-check
fi

MATCH_PATTERN="projects/"
BRANCH=${MANIFEST_BRANCH#*${MATCH_PATTERN}}
WORK_DIR=$BUILDCHECK_BASE_PATH/$PROJECT_NAME/$BRANCH
OUTPUT_DIR=$WORK_DIR/output

LAST_MANIFEST_FILE="$OUTPUT_DIR/manifest_last.xml"
CURRENT_MANIFEST_FILE="$OUTPUT_DIR/manifest.xml"
DIFF_MANIFEST_FILE="$OUTPUT_DIR/updates.xml"
BUILD_LOG="$OUTPUT_DIR/build.log"

if [ -n "$EXCLUDE_REPOS" ]; then
	echo "Exclude repos:"
	echo "$EXCLUDE_REPOS"
	while IFS= read -r line
	do
		[ -n "$REPO_SYNC_IPATTERN" ] && REPO_SYNC_IPATTERN+="|"
		REPO_SYNC_IPATTERN+="$line"
	done <<< "$EXCLUDE_REPOS"
fi

[ "$FRESH_DOWNLOAD" = "yes" ] && rm -rf $WORK_DIR

if [ ! -d "$WORK_DIR" ]; then
	echo -e "\n======== Downloading source code ========"
	mkdir -p $WORK_DIR
	mkdir -p $OUTPUT_DIR
	cd $WORK_DIR
	repo init -u ${MANIFEST_URL} -b ${MANIFEST_BRANCH} --repo-url=git://scgit.amlogic.com/tools/repo.git --no-repo-verify
else
	echo -e "\n======== Syncing source code ========"
	cd $WORK_DIR
	repo forall -i "$REPO_SYNC_IPATTERN" -c git reset -q --hard origin/$BRANCH_NAME
	repo manifest -r -o $LAST_MANIFEST_FILE
fi

repo sync -cq -j8
repo forall -i "$REPO_SYNC_IPATTERN" -c git reset -q --hard origin/$BRANCH_NAME
repo manifest -r -o $CURRENT_MANIFEST_FILE
echo -e "======== Done ========\n"

if [ -f $LAST_MANIFEST_FILE ] && [ -f $CURRENT_MANIFEST_FILE ]; then
	comm -23 <(sort $LAST_MANIFEST_FILE) <(sort $CURRENT_MANIFEST_FILE) > $DIFF_MANIFEST_FILE

	if [ -s $DIFF_MANIFEST_FILE ]; then
		echo "======== Recent Changes ========"

		while IFS= read -r line
		do
			keyline=`echo "$line" | grep 'name=.* path='`

			for keyword in $keyline; do
				[[ $keyword == path=* ]] && repo_path=`echo ${keyword#*path=} | sed 's/\"//g'`
				[[ $keyword == name=* ]] && repo_name=`echo ${keyword#*name=} | sed 's/\"//g'`
				[[ $keyword == revision=* ]] && repo_version=`echo ${keyword#*revision=} | sed 's/\"//g'`
			done

			if [ -d "$repo_path" ]; then
				pushd $repo_path > /dev/null
				echo -e "\nProject $repo_name"
				git log $repo_version..HEAD
				popd > /dev/null
			fi
		done < $DIFF_MANIFEST_FILE
	else
		echo -e "======== Nothing changed since last build ========"
	fi
	rm -f $DIFF_MANIFEST_FILE
fi

if [ -n "$GERRIT_PROJECT" ] && [ -n "$GERRIT_PATCHSET_NUMBER" ] && [ -n "$GERRIT_CHANGE_NUMBER" ]; then
	echo -e "\n======== Applying Gerrit patch $GERRIT_CHANGE_NUMBER on Project $GERRIT_PROJECT ========"
	keyline=`grep "name=\"$GERRIT_PROJECT\"" $CURRENT_MANIFEST_FILE`

	for keyword in $keyline; do
		if [[ $keyword == path=* ]]; then
			repo_path=`echo ${keyword#*path=} | sed 's/\"//g'`
			break;
		fi
	done

	if [ -d "$repo_path" ]; then
		pushd $repo_path > /dev/null
		l2=${GERRIT_CHANGE_NUMBER: -2}
		git fetch ssh://scgit.amlogic.com:29418/${GERRIT_PROJECT} refs/changes/${l2}/${GERRIT_CHANGE_NUMBER}/${GERRIT_PATCHSET_NUMBER}
		git cherry-pick FETCH_HEAD
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
fi

# Manually cherry pick patches
./scripts/cherry_pick.sh

if [[ "$SUBMIT_TYPE" == "release" ]]; then
	echo "========= Building all packages ========"
	./scripts/build_all_pkg.sh > $BUILD_LOG 2>&1
	if [ "$?" -eq 0 ]; then
		echo "======== Done ========"
	else
		cat $BUILD_LOG
		echo -e "\nAborted!"
		exit 1
	fi
else
	echo "========= Building all projects ========"
	./scripts/build_all.sh > $BUILD_LOG 2>&1
	if [ "$?" -eq 0 ]; then
		echo "======== Done ========"
	else
		cat $BUILD_LOG
		echo -e "\nAborted!"
		exit 1
	fi
fi
