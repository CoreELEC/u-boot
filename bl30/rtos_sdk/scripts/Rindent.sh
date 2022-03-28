#! /bin/bash
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

PARAM="-npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1"

function read_dir(){
    for file in `ls $1`
        do
             if [ -d $1"/"$file ]
             then
                read_dir $1"/"$file
             else
                if [ "${file##*.}"x = "c"x ]||[ "${file##*.}"x = "cpp"x ]||[ "${file##*.}"x = "h"x ];then
		    echo $1"/"$file
                    indent $PARAM $1"/"$file -o $1"/"$file
                fi
             fi
        done
}

#excute
read_dir ${1%*/}

