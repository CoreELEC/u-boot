#!/usr/bin/env bash

function move_output_to_top_dir() {
	if [ -f bl30.bin ]
	then
		rm bl30.bin
	fi

	if [ -f demos/amlogic/${arch_dir}/${board}/gcc/RTOSDemo.bin ]
	then
		cp demos/amlogic/${arch_dir}/${board}/gcc/RTOSDemo.bin ./bl30.bin
	fi
}

source scripts/amlogic/mk.sh $1
move_output_to_top_dir
