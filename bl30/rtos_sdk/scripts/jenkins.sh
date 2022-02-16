#!/bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

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

if [ $SUBMIT_TYPE == "daily" -o $SUBMIT_TYPE == "weekly" ];then
	BUILDCHECK_BASE_PATH=/mnt/fileroot/autobuild/workdir/workspace/RTOS/RTOS_SDK/patchbuild
elif [ $SUBMIT_TYPE == "every" ];then
	BUILDCHECK_BASE_PATH=/mnt/fileroot/jenkins/build-check
elif [ $SUBMIT_TYPE == "merge" ];then
	BUILDCHECK_BASE_PATH=/mnt/fileroot/jenkins/build-check
fi

MATCH_PATTERN="projects/"
BRANCH=${BRANCH_NAME#*${MATCH_PATTERN}}
WORK_DIR=$BUILDCHECK_BASE_PATH/$PROJECT_NAME/$BRANCH

LAST_MANIFEST_FILE="manifest_last.xml"
CURRENT_MANIFEST_FILE="manifest.xml"
DIFF_MANIFEST_FILE="updates.xml"

#rm -rf $WORK_DIR
if [ ! -d "$WORK_DIR" ]; then
	echo -e "\n======== Downloading source code ========"
	mkdir -p $WORK_DIR
	cd "$WORK_DIR"
	repo init -u ${MANIFEST_URL} -b ${MANIFEST_BRANCH} --repo-url=git://scgit.amlogic.com/tools/repo.git --no-repo-verify
else
	echo -e "\n======== Syncing source code ========"
	cd "$WORK_DIR"
	repo forall -c git reset -q --hard origin/$BRANCH_NAME
	repo manifest -r -o $LAST_MANIFEST_FILE
fi

repo sync -cq -j8
repo forall -c git reset -q --hard origin/$BRANCH_NAME
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

if [ -n "$GIT_CHERRY_PICK" ]; then
	echo "$GIT_CHERRY_PICK" | while read line
	do
		pattern=":29418/"
		for keyword in $line; do
			if [[ $keyword == *$pattern* ]]; then
				GIT_PROJECT=`echo ${keyword#*${pattern}} | sed 's/\"//g' | sed 's/\/>//g'`
				break;
			fi
		done

		echo -e "\n======== Applying manual patch $GERRIT_CHANGE_NUMBER on Project $GIT_PROJECT ========"
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

echo "========= Building all projects ========"
./scripts/build_all.sh > build.log 2>&1
if [ "$?" -eq 0 ]; then
        echo "======== Done ========"
else
	cat build.log
	echo -e "\nAborted!"
	exit 1
fi

cd -
