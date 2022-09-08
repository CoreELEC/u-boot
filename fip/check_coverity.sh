#!/bin/bash
#
#  author: dongqing.li@amlogic.com
#  2021.11.22 v1.0 init.
#

#############
# function
#############
MK_ARGV=""
NEW_ARGV=""
#COVERITY_PATH="/proj/coverity/cov-analysis-linux64-2020.12/bin"
COV_IM_DIR="./cov-imdir"
COV_RESULT_HTML="./result-html"
HIGH_LEVEL="0"
PATTERN_PATH=""
PATTERN_ENABLE="0"
IS_CMD_STRING=0

#############
# function
#############
function err_exit()
{
	echo "Error: $1"
	exit 1
}

function check_cov_path() {
	echo ""
	echo "check_cov_path: ${MKPATH}"
	echo ""
	if [ -d "${COV_IM_DIR}" ]; then
		rm -rf ${COV_IM_DIR}
	fi
	mkdir -p ${COV_IM_DIR}
	if [ -d "${COV_RESULT_HTML}" ]; then
		rm -rf ${COV_RESULT_HTML}
	fi
}

function run_coverity() {
	echo ""
	echo -e "\e[1;35m[1] run cov-build: $@ \e[0m"
	cov-build --dir ${COV_IM_DIR} $@ || err_exit "cov-build error."
	echo -e "\e[1;35m[1] run cov-build OK. \e[0m"
}

function analysis_coverity() {
	echo ""
	echo -e "\e[1;35m[2] run cov-analyze ... \e[0m"
	if [ ${HIGH_LEVEL} = "1" ]; then
        	if [ "${PATTERN_ENABLE}" = "1" ];then
			cov-analyze --dir ${COV_IM_DIR} --strip-path $MKPATH --all --aggressiveness-level high --fb-max-mem 3072 --tu-pattern "file('/${PATTERN_PATH}')" || err_exit "cov-analyze high level error."
        	else
			cov-analyze --dir ${COV_IM_DIR} --strip-path $MKPATH --all --aggressiveness-level high --fb-max-mem 3072 || err_exit "cov-analyze high level error."
		fi
	else
		if [ "${PATTERN_ENABLE}" = "1" ];then
			cov-analyze --dir ${COV_IM_DIR} --strip-path $MKPATH --all  --tu-pattern "file('/${PATTERN_PATH}')" || err_exit "cov-analyze normal level error."
		else
			cov-analyze --dir ${COV_IM_DIR} --strip-path $MKPATH --all  || err_exit "cov-analyze normal level error."
		fi
	fi
	echo -e "\e[1;35m[2] run cov-analyze OK. \e[0m"

	echo ""
	echo -e "\e[1;35m[3] run cov-format-errors ... \e[0m"
	cov-format-errors --dir ${COV_IM_DIR} --html-output ${COV_RESULT_HTML} --filesort --strip-path $MKPATH -x || err_exit "cov-format-errors error."
	echo -e "\e[1;35m[3] run cov-format-errors OK. \e[0m"
	echo "end."

	rm -rf ${COV_IM_DIR}
}

function show_coverity_result() {
	echo ""
	echo -e "\e[1;35m[html-result] \e[0m"
	echo "you can open the index.html files through a browser, and view code defects."
	echo "path: ${MKPATH}/`basename ${COV_RESULT_HTML}`/index.html"
	echo " "
}

########
# mk_script.sh call function here !
########
function check_coverity() {

	MK_ARGV=$@
	if [ -z "${PATTERN_PATH}" ];then
		echo "pattern_path not set, ignore. "
		PATTERN_ENABLE="0"
	else
		if [ -e "./${PATTERN_PATH}" ];then
			echo "PATTERN_PATH set ok: ${PATTERN_PATH}"
			PATTERN_ENABLE="1"
		else
			echo "PATTERN_PATH invalid, ingore."
			PATTERN_ENABLE="2"
		fi
	fi

	echo -e "\e[1;35m=========== run check_coverity() ===========\e[0m"

	echo "------------------------------------------------------------------"
	echo "coverity raw command  : ./mk $@ "

	# check argv
	result=$(echo ${MK_ARGV} | grep "\-\-cov-high")
	if [[ "$result" != "" ]]; then
		echo "coverity defect level : high"
		HIGH_LEVEL="1"
		NEW_ARGV=`echo "${MK_ARGV//--cov-high/ }"`
	else
		echo "coverity defect level : normal"
		HIGH_LEVEL="0"
		NEW_ARGV=`echo "${MK_ARGV//--cov/ }"`
	fi
	if [ "PATTERN_ENABLE" != "0" ];then
		NEW_ARGV=`echo "${NEW_ARGV//${PATTERN_PATH}/ }"`
	fi
	echo "coverity new command  : ./mk ${NEW_ARGV}"
	MKPATH=`pwd -P`
	echo "coverity run path     : $MKPATH"
	echo "------------------------------------------------------------------"

	# check_cov_path
	check_cov_path

	# run_coverity
	run_coverity "./mk ${NEW_ARGV}"

	# analysis coverity
	analysis_coverity

	# show coverity result info
	show_coverity_result
}

#############
# for bl2/core
#############
function sync_code() {
	#echo "begin sync branch: $1/$2"
	if [ -z $2 ]; then
		err_exit "branch($2) error !"
	fi
	git reset --hard
	cnt=`git branch |grep test1 -c`
	if [ $cnt -eq 0 ]; then
		git checkout -b test1
	else
		git checkout test1
	fi
	cnt=`git branch |grep $2 -c`
	if [ ! $cnt -eq 0 ]; then
		git branch -D $2 > /dev/null
	fi
	git checkout -t $1/$2 || err_exit "git checkout -t $1/$2 faild !"
	git fetch --all
	git reset --hard $1/$2
	git pull
	git branch -D test1
	echo
}

function run_cov_for_bl2_core() {
	# get all support soc
	cd ../ree/plat/
	array=`ls -d *`
	cd - &> /dev/null

	skiped=("common" "fvp" "juno" "golden")
	for item in ${skiped[@]}
	{
		# remove skiped item
		array=${array//${item}/''}
	}

	RESULT='\n'"Build BL2 core for SoC: "${array[@]}'\n'
	echo -e $RESULT

	# loop all soc
	for soc in ${array[@]}
	do
		TEST_BRANCH=projects/$soc
		echo "TEST_BRANCH=:$TEST_BRANCH"

		# prepare code
		sync_code firmware ${TEST_BRANCH}

		# run test
		run_coverity ./mk $soc --dusb
		run_coverity ./mk $soc --dsto
	done
}

function show_help() {

	echo -e "\e[1;35m [usage] \e[0m"
	echo "    /path/to/bootloader/fip/`basename $0` -c [cmd_string] -p [pattern_path] -t"
	echo ""
	echo -e "\e[1;35m [option] \e[0m"
	echo "    -c : cmd string, eg: ./check.sh "
	echo "    -p : detect path, only output errors in this path."
	echo "    -t : top level mode, could detect more errors."
	echo "    -h : show help"
	echo ""
	echo -e "\e[1;35m [example] \e[0m"
	echo "    1) In path [bl2/core]:"
	echo "    	/path/to/bootloader/fip/`basename $0` -c bl2_core"
	echo "    2) In path [bl2/src] [bl2/ree] [bl2/tee] [bl31_1.3/src] [bl32_3.8/src]:"
	echo "    	/path/to/bootloader/fip/`basename $0` -c ./check.sh"
	echo "    3) In path [bl33/v2015] [bl33/v2019]:"
	echo "    	/path/to/bootloader/fip/`basename $0` -c ./check_compile.sh"
	echo ""
	return
}

#############
# main
#############
function main() {
	if [[ "$0" =~ "coverity" ]]; then
		echo "cmd string: $0 $@"
		if [ $# -lt 1 ]; then
			show_help
		fi
	else
		return
	fi

	while getopts c:C:p:P:t:T:hH opt; do
			case ${opt} in
			c|C)
					CMD_STRING=${OPTARG}
					IS_CMD_STRING=1
					;;
			p|P)
					PATTERN_PATH=${OPTARG}
					;;
			t|T)
					HIGH_LEVEL="1"
					;;
			h|H)
					show_help
					;;
			*)
					show_help
					;;
			esac
	done

	if [ $IS_CMD_STRING -eq 1 ]; then
		# check coverity path
		check_cov_path

		# run_coverity
		if [ ${CMD_STRING} = "bl2_core" ]; then
			run_cov_for_bl2_core
		else
			run_coverity ${CMD_STRING}
		fi

		# analysis coverity
		analysis_coverity

		# show coverity result path
		show_coverity_result
	fi
}

#
# start here.
#
MKPATH=`pwd -P`
main $@
