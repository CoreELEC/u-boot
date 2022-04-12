#! /bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

PARAM="-npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -bad"

function read_dir(){
	for file in `ls $1`
	do
		if [ -d $1"/"$file ]
		then
			read_dir $1"/"$file
		elif [ "${file##*.}"x = "c"x ]||[ "${file##*.}"x = "cpp"x ]||[ "${file##*.}"x = "h"x ];then
			echo $1"/"$file
			indent $PARAM $1"/"$file -o $1"/"$file
		fi
	done
}

#excute
if [ -d $1 ]; then
	read_dir ${1%*/}
elif [ -f $1 ]; then
	if [ "${1##*.}"x = "c"x ]||[ "${1##*.}"x = "cpp"x ]||[ "${1##*.}"x = "h"x ];then
		echo $1
		indent $PARAM $1 -o $1
	fi
fi
