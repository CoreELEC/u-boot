#!/bin/bash

user_input=
user_rsa=
user_aes=
user_out=
soc=
hash_ver=2

while getopts "s:h:z:p:r:a:uno:" opt; do
  case $opt in
    s) readonly soc="$OPTARG" ;;
    z) readonly user_package="$OPTARG" ;;
    p) readonly user_input="$OPTARG" ;;
    r) readonly user_rsa="$OPTARG" ;;
    a) readonly user_aes="$OPTARG" ;;
    o) readonly user_out="$OPTARG" ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 2
      ;;
  esac
done

tool_type=gxl

if [ ${soc} == "g12a" ]; then
tool_type=g12a
fi

if [ ${soc} == "g12b" ]; then
tool_type=g12a
fi

if [ $soc == "gxl" ]; then
hash_ver=1
fi

readonly tools_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
kernel_encrypt_signed="${tools_dir}/signing-tool-${tool_type}-dev/kernel.encrypt.signed.bash"

INPUTDIR=
OUTPUTDIR=
RSAKEYDIR=
AESKEYDIR=
temp_dir=

if [[ -f $user_package ]]; then  
	temp_dir="$(dirname $user_package )"/"$(basename $user_package)"-`date +%Y%m%d-%H%M%S`
	mkdir -p $temp_dir
	if [[ -d $temp_dir ]]; then  
	unzip $user_package -d $temp_dir >& /dev/null
	if [[ -d $user_input ]]; then
		echo "error!user package and input conflicts! Only one set is legal!" 
		exit 1;
	else
		user_input=$temp_dir
	fi
	fi
fi

if [[ -d $user_input ]]; then  
	INPUTDIR=$user_input
fi
if [[ -d $user_rsa ]]; then
  RSAKEYDIR=$user_rsa
fi
if [[ -d $user_aes ]]; then  
	AESKEYDIR=$user_aes
fi
if [[ ! -z $user_out ]]; then  
	OUTPUTDIR=$user_out
fi

mkdir -p ${OUTPUTDIR}
echo "--- output to ${OUTPUTDIR}---"


#to sign uboot and output to ${OUTPUTDIR}
if [ -e ${INPUTDIR}/bl2_new.bin ]; then
	${tools_dir}/amlogic-sign-${tool_type}.sh -p ${INPUTDIR} -r ${RSAKEYDIR} -a ${AESKEYDIR} -o ${OUTPUTDIR} -h ${hash_ver} -s ${tool_type}
fi


#check and sign kernel
if [ -e ${INPUTDIR}/boot.img ]; then
	"$kernel_encrypt_signed" ${INPUTDIR}/boot.img ${RSAKEYDIR} ${OUTPUTDIR}/boot.img.encrypt
fi

#check and sign recovery
if [ -e ${INPUTDIR}/recovery.img ]; then
	"$kernel_encrypt_signed" ${INPUTDIR}/recovery.img ${RSAKEYDIR} ${OUTPUTDIR}/recovery.img.encrypt	
fi

#check and sign dtb
if [ -e ${INPUTDIR}/dtb.img ]; then
	"$kernel_encrypt_signed" ${INPUTDIR}/dtb.img ${RSAKEYDIR} ${OUTPUTDIR}/dtb.img.encrypt	
fi

#check and sign dtb with another name
if [ -e ${INPUTDIR}/dt.img ]; then
	"$kernel_encrypt_signed" ${INPUTDIR}/dt.img ${RSAKEYDIR} ${OUTPUTDIR}/dt.img.encrypt	
fi

if [ -d $temp_dir ]; then
	rm -fr $temp_dir
fi