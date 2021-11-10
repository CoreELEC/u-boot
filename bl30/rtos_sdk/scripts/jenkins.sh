#!/bin/bash

export PATH=/opt/cmake-3.18.4-Linux-x86_64/bin/:$PATH
export PATH=/proj/coverity/cov-analysis/bin/:$PATH
export LM_LICENSE_FILE=/mnt/fileroot/jenkins/Xplorer_key.txt:$LM_LICENSE_FILE
export PATH=/opt/xtensa/XtDevTools/install/tools/RG-2018.9-linux/XtensaTools/bin:$PATH
export XTENSA_SYSTEM=/opt/xtensa/XtDevTools/install/builds/RG-2018.9-linux/Amlogic_v0/config
export XTENSA_CORE=Amlogic_v0

if [ -z "$MANIFEST_URL" ] || [ -z "$MANIFEST_BRANCH" ] || [ -z "$PROJECT_NAME" ] || [ -z "$BRANCH_NAME" ]; then
	echo "param must not null"
	exit 1
fi

if [ $SUBMIT_TYPE = "daily" -o $SUBMIT_TYPE = "weekly" ];then
	BUILDCHECK_BASE_PATH=/mnt/fileroot/jenkins/auto-build
elif [ $SUBMIT_TYPE = "every" ];then
	BUILDCHECK_BASE_PATH=/mnt/fileroot/jenkins/build-check
elif [ $SUBMIT_TYPE = "merge" ];then
	BUILDCHECK_BASE_PATH=/mnt/fileroot/jenkins/build-check
fi

MATCH_PATTERN="projects/"
BRANCH=${BRANCH_NAME#*${MATCH_PATTERN}}
WORK_DIR=$BUILDCHECK_BASE_PATH/$PROJECT_NAME/$BRANCH

LAST_MANIFEST_FILE="manifest_last.xml"
CURRENT_MANIFEST_FILE="manifest.xml"
DIFF_MANIFEST_FILE="updates.xml"

rm -rf $WORK_DIR
if [ ! -d "$WORK_DIR" ]; then
	echo -e "\n======== Downloading source code ========"
	mkdir -p $WORK_DIR
	cd "$WORK_DIR"
	repo init -u ${MANIFEST_URL} -b ${MANIFEST_BRANCH}  --repo-url=git://scgit.amlogic.com/tools/repo.git --no-repo-verify
else
	echo -e "\n======== Syncing source code ========"
    cd "$WORK_DIR"
    repo forall -c git reset --hard origin/$BRANCH_NAME
	repo manifest -r -o $LAST_MANIFEST_FILE
fi

repo sync -cq -j8
repo forall -c git reset --hard origin/$BRANCH_NAME
repo manifest -r -o $CURRENT_MANIFEST_FILE
echo -e "======== Downloading/Syncing Done ========\n"

echo -e "======== Recent Changes ========\n"
comm -23 <(sort $LAST_MANIFEST_FILE) <(sort $CURRENT_MANIFEST_FILE) > $DIFF_MANIFEST_FILE

while IFS= read -r line
do
	keyword=`echo "$line" | grep 'name=.* path='`

	if [ -n "$keyword" ]; then
		repo_name=`echo "$keyword" | awk '{print $2}'`
		repo_name=`echo ${repo_name#*name=} | sed 's/\"//g'`
		repo_path=`echo "$keyword" | awk '{print $3}'`
		repo_path=`echo ${repo_path#*path=} | sed 's/\"//g'`
		repo_version=`echo "$keyword" | awk '{print $4}'`
		repo_version=`echo ${repo_version#*revision=} | sed 's/\"//g'`

		pushd $repo_path > /dev/null
		echo -e "\nProject $repo_name"
		git log $repo_version..HEAD
		popd > /dev/null
	fi
done < $DIFF_MANIFEST_FILE

parse_list=`echo ${PROFILE}|tr ',' ' '`
for single in $parse_list; do
	board=`echo "$single"|awk -F ':' '{print $1}'`
	product=`echo "$single"|awk -F ':' '{print $2}'`
	if [ -z "$board" ] || [ -z "$product" ]; then
		if [ -z "$board" ]; then
			echo "No board specified!"
		fi
		if [ -z "$product" ]; then
			echo "No product specified!"
		fi
		exit 1
	fi

	echo "\n========= Building $board $product ========="
	source ./scripts/env.sh $board $product
	make distclean 2>&1
	make 2>&1
	if [ "$?" -eq 0 ];then
	echo "========= Building successful =========\n"
	else
	echo "========= Building failed =========\n"
	exit 1
	fi
done

cd -
