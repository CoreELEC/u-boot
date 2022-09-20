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

LAST_MANIFEST="$OUTPUT_DIR/last_manifest.xml"
CURRENT_MANIFEST="$OUTPUT_DIR/curr_manifest.xml"
DIFF_MANIFEST="$OUTPUT_DIR/diff_manifest.xml"

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
	if [ -n "$REPO_SYNC_IPATTERN" ]; then
		repo forall -i "$REPO_SYNC_IPATTERN" -c git reset -q --hard origin/$BRANCH_NAME
	else
		repo forall -c git reset -q --hard origin/$BRANCH_NAME
	fi
	repo manifest -r -o $LAST_MANIFEST
fi

repo sync -cq -j8 --prune
[ "$?" -ne 0 ] && cd - && echo "Sync error! will do fresh download next time" && exit 1
if [ -n "$REPO_SYNC_IPATTERN" ]; then
	repo forall -i "$REPO_SYNC_IPATTERN" -c git reset -q --hard origin/$BRANCH_NAME
else
	repo forall -c git reset -q --hard origin/$BRANCH_NAME
fi
repo manifest -r -o $CURRENT_MANIFEST
echo -e "======== Done ========\n"

if [ -f $LAST_MANIFEST ] && [ -f $CURRENT_MANIFEST ]; then
	comm -23 <(sort $LAST_MANIFEST) <(sort $CURRENT_MANIFEST) > $DIFF_MANIFEST

	if [ -s $DIFF_MANIFEST ]; then
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
		done < $DIFF_MANIFEST
		echo -e "================\n"
	else
		echo -e "======== Nothing changed since last build ========\n"
	fi
fi

# Cherry pick patches
source scripts/cherry_pick.sh

# Generate Jenkins trigger
[ "$SUBMIT_TYPE" = "release" ] && source gen_jenkins_trigger.sh

if [[ "$MANIFEST_BRANCH" == "$BRANCH_NAME" ]]; then
	source scripts/build_all_pkg.sh
else
	source scripts/build_all.sh
fi
