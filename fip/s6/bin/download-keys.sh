 #!/bin/bash

EXEC_BASEDIR=$(dirname $(readlink -f $0))
BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

REPO_ADDR_HEAD="ssh://scgit.amlogic.com:29418/security"

key_type=$1
soc=$2
key_name=$3
key_path=$4

#chipset repo-name
CHIPSET_REMOTE_NAME="/keys/${key_type}/${soc}/chipset/keys"
CHIPSET_LOCAL_NAME="/keys/${key_type}/${soc}/chipset"

#device repo-name
DEVICE_REPO_FOLDER="/keys/${key_type}/${soc}/device/"
DEVICE_REPO_NAME[0]="boot-blobs"
DEVICE_REPO_NAME[1]="fip"

if [ ${key_name} == "chipset"  ]; then
	if [ ! -d ${BASEDIR_TOP}/${CHIPSET_LOCAL_NAME} ]; then
		git clone ${REPO_ADDR_HEAD}${CHIPSET_REMOTE_NAME} ${BASEDIR_TOP}/${CHIPSET_LOCAL_NAME}
	fi
elif [ ${key_name} == "device"  ]; then
	for NAME in ${DEVICE_REPO_NAME[@]};
	do
		if [ ! -d ${key_path}/${NAME} ]; then
			git clone ${REPO_ADDR_HEAD}${DEVICE_REPO_FOLDER}${NAME} ${key_path}/${NAME}
		fi
	done
fi