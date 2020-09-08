 #!/bin/bash

EXEC_BASEDIR=$(dirname $(readlink -f $0))
BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

REPO_ADDR_HEAD="ssh://scgit.amlogic.com:29418/security"
#chipset repo-name
CHIPSET_REPO_NAME[0]="/keys/dev-keys/sc2/chipset/bl2/aes"
CHIPSET_REPO_NAME[1]="/keys/dev-keys/sc2/chipset/bl2/rsa"
CHIPSET_REPO_NAME[2]="/keys/dev-keys/sc2/chipset/bl31/aes"
CHIPSET_REPO_NAME[3]="/keys/dev-keys/sc2/chipset/bl31/rsa"
CHIPSET_REPO_NAME[4]="/keys/dev-keys/sc2/chipset/bl32/aes"
CHIPSET_REPO_NAME[5]="/keys/dev-keys/sc2/chipset/bl32/rsa"
CHIPSET_REPO_NAME[6]="/keys/dev-keys/sc2/chipset/bl40/aes"
CHIPSET_REPO_NAME[7]="/keys/dev-keys/sc2/chipset/bl40/rsa"

#device repo-name
DEVICE_REPO_FOLDER="/keys/dev-keys/sc2/device/"
DEVICE_REPO_NAME[0]="boot-blobs"
DEVICE_REPO_NAME[1]="fip"

if [ $1 == "chipset"  ]; then
	for NAME in ${CHIPSET_REPO_NAME[@]};
	do
		if [ ! -d ${BASEDIR_TOP}/${NAME} ]; then
			git clone ${REPO_ADDR_HEAD}${NAME} ${BASEDIR_TOP}/${NAME}
		fi
	done
elif [ $1 == "device"  ]; then
	for NAME in ${DEVICE_REPO_NAME[@]};
	do
		if [ ! -d $2/${NAME} ]; then
			git clone ${REPO_ADDR_HEAD}${DEVICE_REPO_FOLDER}${NAME} $2/${NAME}
		fi
	done
fi