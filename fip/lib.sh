#!/bin/bash

DEBUG_PRINT=0


declare GIT_OPERATE_INFO=""

function dbg() {
	if [ 0 != ${DEBUG_PRINT} ]; then
		echo "$1"
	fi
}

declare str_use=""
# filter means get useful information
function string_filter() {
	# #1 origin str, #2 filter str, #3 split char, #4 which section
	local str_origin=$1
	local str_filter=$2
	local str_split=$3
	str_origin=${str_origin#*${str_filter}} # filter
	IFS=${str_split} read -ra DATA <<< "$str_origin"
	str_use=${DATA[$4]}
}

function get_versions() {
	echo "Get version info"
	# read manifest, get each blx information
	while read -r line || [[ -n $line ]]; do
		string_filter "${line}" "dest-branch=" '"' 1
		GIT_INFO[0]=${str_use}
		string_filter "${line}" "path=" '"' 1
		GIT_INFO[1]=${str_use}
		string_filter "${line}" "revision=" '"' 1
		GIT_INFO[2]=${str_use}
		string_filter "${line}" "name=" '"' 1
		GIT_INFO[3]=${str_use}
		string_filter "${line}" "remote=" '"' 1
		GIT_INFO[4]=${str_use}
		# if this line doesn't contain any info, skip it
		if [ "${GIT_INFO[2]}" == "" ]; then
			continue
		fi
		#echo "${GIT_INFO[0]} ${GIT_INFO[1]} ${GIT_INFO[2]} ${GIT_INFO[3]} ${GIT_INFO[4]}"
		#echo ${BLX_NAME[@]}
		#echo ${BLX_SRC_FOLDER[@]}
		for loop in ${!BLX_NAME[@]}; do
			if [ "${GIT_INFO[1]}" == "${BLX_SRC_FOLDER[$loop]}" ]; then
				CUR_REV[$loop]=${GIT_INFO[2]}
				#CUR_BIN_BRANCH[$loop]=${GIT_INFO[0]}
				echo -n "name:${BLX_NAME[$loop]}, path:${BLX_SRC_FOLDER[$loop]}, "
				if [ "${CUR_REV[$loop]}" == "${GIT_INFO[0]}" ]; then
					# if only specify branch name, not version, use latest binaries under bin/ folders
					git_operate ${BLX_BIN_FOLDER[loop]} log --pretty=oneline -1
					git_msg=${GIT_OPERATE_INFO}
					IFS=' ' read -ra DATA <<< "$git_msg"
					CUR_REV[$loop]=${DATA[2]}
					echo -n "revL:${CUR_REV[$loop]} "
				else
					echo -n "rev:${CUR_REV[$loop]} "
				fi
				echo "@ ${GIT_INFO[0]}"
			fi
		done
	done < "$MANIFEST"
}

function git_operate() {
	# $1: path, $2: other parameters
	GIT_OPERATE_INFO=`git --git-dir $1/.git --work-tree=$1 ${@:2}`
	dbg "${GIT_OPERATE_INFO}"
}

function git_operate2() {
	# use -C. for pull use. don't know why [git_operate pull] doesn't work, server git update?
	# $1: path, $2: other parameters
	GIT_OPERATE_INFO="`git -C \"$1\" ${@:2}`"
	#echo "${GIT_OPERATE_INFO}"
}

function get_blx_bin() {
	# $1: current blx index
	index=$1
	git_operate ${BLX_BIN_FOLDER[index]} log --pretty=oneline

	git_msg=${GIT_OPERATE_INFO}
	BLX_READY[${index}]="false"
	mkdir -p ${FIP_BUILD_FOLDER}

	# get version log line by line, compare with target version
	line_num=0
	while read line;
	do
		IFS=' ' read -ra DATA <<< "$line"
		# v1-fix support short-id
		#echo "${CUR_REV[$index]:0:7} - ${DATA[2]:0:7}"
		if [ "${CUR_REV[$index]:0:7}" == "${DATA[2]:0:7}" ]; then
			BLX_READY[${index}]="true"
			dbg "blxbin:${DATA[0]} blxsrc:  ${DATA[2]}"
			dbg "blxbin:${DATA[0]} blxsrc-s:${DATA[2]:0:7}"
			# reset to history version
			#git --git-dir ${BLX_BIN_FOLDER[index]}/.git --work-tree=${BLX_BIN_FOLDER[index]} reset ${DATA[0]} --hard
			git_operate2 ${BLX_BIN_FOLDER[index]} reset ${DATA[0]} --hard
			# copy binary file
			if [ "bl32" == "${BLX_NAME[$index]}" ]; then
				# bl32 is optional
				if [ "y" == "${CONFIG_NEED_BL32}" ]; then
					cp ${BLX_BIN_FOLDER[index]}/${CUR_SOC}/${BLX_BIN_NAME[index]} ${FIP_BUILD_FOLDER} -f
					if [ "y" == "${CONFIG_FIP_IMG_SUPPORT}" ]; then
						cp ${BLX_BIN_FOLDER[index]}/${CUR_SOC}/${BLX_IMG_NAME[index]} ${FIP_BUILD_FOLDER} 2>/dev/null
					fi
				fi
			else
				cp ${BLX_BIN_FOLDER[index]}/${CUR_SOC}/${BLX_BIN_NAME[index]} ${FIP_BUILD_FOLDER} -f
				if [ "y" == "${CONFIG_FIP_IMG_SUPPORT}" ]; then
					cp ${BLX_BIN_FOLDER[index]}/${CUR_SOC}/${BLX_IMG_NAME[index]} ${FIP_BUILD_FOLDER} 2>/dev/null
				fi
			fi
			# undo reset
			if [ 0 -ne ${line_num} ]; then
				# this is not latest version, can do reset. latest version doesn't have 'git reflog'
				#git --git-dir ${BLX_BIN_FOLDER[index]}/.git --work-tree=${BLX_BIN_FOLDER[index]} reset 'HEAD@{1}' --hard
				git_operate2 ${BLX_BIN_FOLDER[index]} reset 'HEAD@{1}' --hard
			fi
			break
		fi
		line_num=$((line_num+1))
	done <<< "${git_msg}"
	if [ "true" == ${BLX_READY[${index}]} ]; then
		echo "Get ${BLX_NAME[$index]} from ${BLX_BIN_FOLDER[$index]}... done"
	else
		echo -n "Get ${BLX_NAME[$index]} from ${BLX_BIN_FOLDER[$index]}... failed"
		if [ "true" == ${BLX_NEEDFUL[$index]} ]; then
			echo "... abort"
			exit -1
		else
			echo ""
		fi
	fi
	return 0;
}