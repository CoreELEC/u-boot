 #!/bin/bash

EXEC_BASEDIR=$(dirname $(readlink -f $0))
BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

REPO_ADDR_HEAD="ssh://scgit.amlogic.com:29418/security"
#repo-name
REPO_NAME[0]="/keys/dev-keys/sc2/chipset/bl2/aes"
REPO_NAME[1]="/keys/dev-keys/sc2/chipset/bl2/rsa"
REPO_NAME[2]="/keys/dev-keys/sc2/chipset/bl31/aes"
REPO_NAME[3]="/keys/dev-keys/sc2/chipset/bl31/rsa"
REPO_NAME[4]="/keys/dev-keys/sc2/chipset/bl32/aes"
REPO_NAME[5]="/keys/dev-keys/sc2/chipset/bl32/rsa"
REPO_NAME[6]="/keys/dev-keys/sc2/chipset/bl40/aes"
REPO_NAME[7]="/keys/dev-keys/sc2/chipset/bl40/rsa"


for NAME in ${REPO_NAME[@]};
do
	if [ ! -d ${BASEDIR_TOP}/${NAME} ]; then
		git clone ${REPO_ADDR_HEAD}${NAME} ${BASEDIR_TOP}/${NAME}
	fi
done
