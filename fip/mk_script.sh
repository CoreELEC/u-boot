#!/bin/bash
#
#  author: xiaobo.gu@amlogic.com
#  2016.09.28
#  2016.12.01-2016.12.20 Update for bootloader repo
#

DEBUG_PRINT=0

BUILD_FOLDER="build/"
FIP_BUILD_FOLDER="fip/build/"
FIP_FOLDER="fip/"
MAIN_FOLDER=""
MANIFEST=".repo/manifest.xml"

# include uboot pre-build macros
declare SOURCE_FILE=("build/.config")
declare CONFIG_FILE=("build/include/autoconf.mk")

# static
declare -a BLX_NAME=("bl2" "bl30" "bl31" "bl32")
declare -a BLX_SRC_FOLDER=("bl2/src" "bl30/src" "bl31/src" "bl32/src" "bl33")
declare -a BLX_BIN_FOLDER=("bl2/bin" "bl30/bin" "bl31/bin" "bl32/bin")
declare -a BLX_BIN_NAME=("bl2.bin" "bl30.bin" "bl31.bin" "bl32.bin")
declare -a BLX_IMG_NAME=("NULL" "NULL" "bl31.img" "bl32.img")
declare -a BLX_NEEDFUL=("true" "true" "true" "false")

# blx priority. null: default, source: src code, others: bin path
declare -a BIN_PATH=("null" "null" "null" "null")
# variables
# current branch/path/rev/name/remote in xml
declare -a GIT_INFO=("branch", "path", "rev", "name", "remote")
declare -a CUR_REV # current version of each blx
declare -a BLX_READY=("false", "false", "false", "false") # blx build/get flag

CUR_SOC=""
GIT_OPERATE_INFO=""
BOARD_DIR=""

# toolchain defines
declare AARCH64_TOOL_CHAIN="/opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/aarch64-none-elf-"
declare AARCH32_TOOL_CHAIN="arm-none-eabi-"

function dbg() {
  if [ 0 != ${DEBUG_PRINT} ]; then
    echo "$1"
  fi
}

function git_operate() {
  # $1: path, $2: other parameters
  GIT_OPERATE_INFO=`git --git-dir $1/.git --work-tree=$1 ${@:2}`
  dbg "${GIT_OPERATE_INFO}"
}

function pre_build_uboot() {
  cd ${BLX_SRC_FOLDER[4]}
  echo -n "Compile config: "
  echo "$1"
  make distclean &> /dev/null
  make $1'_config' &> /dev/null
  if [ $? != 0 ]
  then
    echo "Pre-build failed! exit!"
    cd ${MAIN_FOLDER}
    exit -1
  else
    if [ ! -e ${SOURCE_FILE} ]; then
      echo "${SOURCE_FILE} doesn't exist!"
      cd ${MAIN_FOLDER}
      return
    else
      source ${SOURCE_FILE}
    fi
    CUR_SOC=${CONFIG_SYS_SOC}
  fi
  cd ${MAIN_FOLDER}
}

function build_uboot() {
  echo "Build uboot...Please Wait..."
  mkdir -p ${FIP_BUILD_FOLDER}
  cd ${BLX_SRC_FOLDER[4]}
  make -j #&> /dev/null
  ret=$?
  source ${CONFIG_FILE} &> /dev/null # ignore warning/error
  cd ${MAIN_FOLDER}
  if [ "y" == "${CONFIG_SUPPORT_CUSOTMER_BOARD}" ]; then
    BOARD_DIR="customer/board/${CONFIG_SYS_BOARD}"
  else
    BOARD_DIR="${CONFIG_BOARDDIR}"
  fi
  if [ 0 -ne $ret ]; then
    echo "Error: U-boot build failed... abort"
    exit -1
  else
    cp ${BLX_SRC_FOLDER[4]}/build/u-boot.bin ${FIP_BUILD_FOLDER}bl33.bin -f
    cp ${BLX_SRC_FOLDER[4]}/build/scp_task/bl301.bin ${FIP_BUILD_FOLDER} -f
    cp ${BLX_SRC_FOLDER[4]}/build/${BOARD_DIR}/firmware/bl21.bin ${FIP_BUILD_FOLDER} -f
    cp ${BLX_SRC_FOLDER[4]}/build/${BOARD_DIR}/firmware/acs.bin ${FIP_BUILD_FOLDER} -f
  fi
}

function uboot_config_list() {
  folder_board="${BLX_SRC_FOLDER[4]}/board/amlogic"
  echo "      ******Amlogic Configs******"
  for file in ${folder_board}/*; do
    temp_file=`basename $file`
    #echo "$temp_file"
    if [ -d ${folder_board}/${temp_file} ] && [ "$temp_file" != "defconfigs" ] && [ "$temp_file" != "configs" ];then
      echo "          ${temp_file}"
    fi
  done

  customer_folder="${BLX_SRC_FOLDER[4]}/customer/board"
  if [ -e ${customer_folder} ]; then
    echo "      ******Customer Configs******"
    for file in ${customer_folder}/*; do
      temp_file=`basename $file`
      if [ -d ${customer_folder}/${temp_file} ] && [ "$temp_file" != "defconfigs" ] && [ "$temp_file" != "configs" ];then
        echo "          ${temp_file}"
      fi
    done
  fi
  echo "      ***************************"
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
    for loop in ${!BLX_NAME[@]}; do
      if [ "${GIT_INFO[1]}" == "${BLX_SRC_FOLDER[$loop]}" ]; then
        #echo "found match index x: $loop, blx: ${DATA[0]}, rev: ${DATA[1]}"
        CUR_REV[$loop]=${GIT_INFO[2]}
        #CUR_BIN_BRANCH[$loop]=${GIT_INFO[0]}
        echo "name:${BLX_NAME[$loop]}, path:${BLX_SRC_FOLDER[$loop]}, rev:${CUR_REV[$loop]} @ ${GIT_INFO[0]}"
      fi
    done
  done < "$MANIFEST"
}

function build_bl2() {
  echo -n "Build bl2...Please wait..."
  local target="$1/build/$3/release/bl2.bin"
  # $1: src_folder, $2: bin_folder, $3: soc
  cd $1
  export CROSS_COMPILE=${AARCH64_TOOL_CHAIN}
  make PLAT=$3 distclean &> /dev/null
  make PLAT=$3 &> /dev/null
  if [ $? != 0 ]; then
    cd ${MAIN_FOLDER}
    echo "Error: Build bl2 failed... abort"
    exit -1
  fi
  cd ${MAIN_FOLDER}
  cp ${target} $2 -f
  echo "done"
  return
}

function build_bl30() {
  echo -n "Build bl30...Please wait..."
  local target="$1/bl30.bin"
  # $1: src_folder, $2: bin_folder, $3: soc
  cd $1
  export CROSS_COMPILE=${AARCH32_TOOL_CHAIN}
  local soc=$3
  if [ $soc == "gxtvbb" ]; then
    soc="gxtvb"
  fi
  make clean BOARD=$soc &> /dev/null
  make BOARD=$soc &> /dev/null
  if [ $? != 0 ]; then
    cd ${MAIN_FOLDER}
    echo "Error: Build bl30 failed... abort"
    exit -1
  fi
  rm ./bl30.bin -f
  cp build/$soc/ec.RW.bin ./
  mv ./ec.RW.bin bl30.bin
  cd ${MAIN_FOLDER}
  cp ${target} $2 -f
  echo "done"
  return
}

function build_bl31() {
  echo -n "Build bl31...Please wait... "
  # $1: src_folder, $2: bin_folder, $3: soc
  cd $1
  export CROSS_COMPILE=${AARCH64_TOOL_CHAIN}
  CONFIG_SPD="opteed"
  #CONFIG_SPD="none"
  local soc=$3
  if [ $soc == "gxtvbb" ] || [ $soc == "gxb" ]; then
    soc="gxbb"
  elif [ $soc == "txl" ]; then
    soc="gxl"
  fi
  make PLAT=${soc} SPD=${CONFIG_SPD} realclean &> /dev/null
  make PLAT=${soc} SPD=${CONFIG_SPD} DEBUG=1 V=1 all &> /dev/null
  if [ $? != 0 ]; then
    cd ${MAIN_FOLDER}
    echo "Error: Build bl31 failed... abort"
    exit -1
  fi
  cd ${MAIN_FOLDER}
  local target="$1/build/${soc}/debug/bl31.bin"
  local target2="$1/build/${soc}/debug/bl31.img"
  cp ${target} $2 -f
  cp ${target2} $2 -f
  echo "done"
  return
}

function build_bl32() {
  echo -n "Build bl32...Please wait... "
  # $1: src_folder, $2: bin_folder, $3: soc
  cd $1
  # todo
  cd ${MAIN_FOLDER}
  echo "done"
  return
}

function build_blx_src() {
  # compile $name $src_folder $bin_folder $soc
  local name=$1
  local src_folder=$2
  local bin_folder=$3
  local soc=$4
  #dbg "compile - name: ${name}, src_folder: ${src_folder}, bin_folder: ${bin_folder}, soc: ${soc}"
  if [ $name == ${BLX_NAME[0]} ]; then
    # bl2
    build_bl2 $src_folder $bin_folder $soc
  elif [ $name == ${BLX_NAME[1]} ]; then
    # bl30
    build_bl30 $src_folder $bin_folder $soc
  elif [ $name == ${BLX_NAME[2]} ]; then
    # bl31
    build_bl31 $src_folder $bin_folder $soc
  elif [ $name == ${BLX_NAME[3]} ]; then
    # bl32
    build_bl32 $src_folder $bin_folder $soc
  fi
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
    if [ "${CUR_REV[$index]:0:7}" == "${DATA[2]:0:7}" ]; then
      BLX_READY[${index}]="true"
      dbg "blxbin:${DATA[0]} blxsrc:  ${DATA[2]}"
      dbg "blxbin:${DATA[0]} blxsrc-s:${DATA[2]:0:7}"
      # reset to history version
      #git --git-dir ${BLX_BIN_FOLDER[index]}/.git --work-tree=${BLX_BIN_FOLDER[index]} reset ${DATA[0]} --hard
      git_operate ${BLX_BIN_FOLDER[index]} reset ${DATA[0]} --hard
      # copy binary file
      cp ${BLX_BIN_FOLDER[index]}/${CUR_SOC}/${BLX_BIN_NAME[index]} ${FIP_BUILD_FOLDER} -f
      if [ "y" == "${CONFIG_FIP_IMG_SUPPORT}" ]; then
        cp ${BLX_BIN_FOLDER[index]}/${CUR_SOC}/${BLX_IMG_NAME[index]} ${FIP_BUILD_FOLDER} 2>/dev/null
      fi
      # undo reset
      if [ 0 -ne ${line_num} ]; then
        # this is not latest version, can do reset. latest version doesn't have 'git reflog'
        #git --git-dir ${BLX_BIN_FOLDER[index]}/.git --work-tree=${BLX_BIN_FOLDER[index]} reset 'HEAD@{1}' --hard
        git_operate ${BLX_BIN_FOLDER[index]} reset 'HEAD@{1}' --hard
      fi
      break
    fi
    line_num=$((line_num+1))
  done <<< "${git_msg}"
  if [ "true" == ${BLX_READY[${index}]} ]; then
    echo "Get ${BLX_BIN_NAME[$index]} from ${BLX_BIN_FOLDER[$index]}... done"
  else
    echo -n "Get ${BLX_BIN_NAME[$index]} from ${BLX_BIN_FOLDER[$index]}... failed"
    if [ "true" == ${BLX_NEEDFUL[$index]} ]; then
      echo "... abort"
      exit -1
    else
      echo ""
    fi
  fi
  return 0;
}

function build_blx() {
  mkdir -p ${FIP_BUILD_FOLDER}
  for loop in ${!BLX_NAME[@]}; do
    dbg "BIN_PATH[${loop}]: ${BIN_PATH[loop]}"
    if [ "null" == ${BIN_PATH[loop]} ]; then
      get_blx_bin ${loop}
    elif [ "source" == ${BIN_PATH[loop]} ]; then
      dbg "Build blx source code..."
      build_blx_src ${BLX_NAME[loop]} ${BLX_SRC_FOLDER[loop]} ${FIP_BUILD_FOLDER} ${CUR_SOC}
    else
      if [ ! -e ${BIN_PATH[loop]} ]; then
        echo "Error: ${BIN_PATH[loop]} doesn't exist... abort"
        exit -1
      else
        cp ${BIN_PATH[loop]} ${FIP_BUILD_FOLDER}${BLX_BIN_NAME[$loop]} -f
        echo "Get ${BLX_BIN_NAME[$loop]} from ${BIN_PATH[loop]}... done"
      fi
    fi
  done
  return
}

function fix_blx() {
  #bl2 file size 41K, bl21 file size 3K (file size not equal runtime size)
  #total 44K
  #after encrypt process, bl2 add 4K header, cut off 4K tail

  #bl30 limit 41K
  #bl301 limit 12K
  #bl2 limit 41K
  #bl21 limit 3K, but encrypt tool need 48K bl2.bin, so fix to 7168byte.

  #$7:name flag
  if [ "$7" = "bl30" ]; then
    declare blx_bin_limit=40960   # PD#132613 2016-10-31 update, 41984->40960
    declare blx01_bin_limit=13312 # PD#132613 2016-10-31 update, 12288->13312
  elif [ "$7" = "bl2" ]; then
    declare blx_bin_limit=41984
    declare blx01_bin_limit=7168
  else
    echo "blx_fix name flag not supported!"
    exit 1
  fi

  # blx_size: blx.bin size, zero_size: fill with zeros
  declare -i blx_size=`du -b $1 | awk '{print int($1)}'`
  declare -i zero_size=$blx_bin_limit-$blx_size
  dd if=/dev/zero of=$2 bs=1 count=$zero_size
  cat $1 $2 > $3
  rm $2

  declare -i blx01_size=`du -b $4 | awk '{print int($1)}'`
  declare -i zero_size_01=$blx01_bin_limit-$blx01_size
  dd if=/dev/zero of=$2 bs=1 count=$zero_size_01
  cat $4 $2 > $5

  cat $3 $5 > $6

  rm $2
}

copy_bootloader() {
  mkdir -p ${BUILD_FOLDER}
  cp ${FIP_BUILD_FOLDER}u-boot.bin ${BUILD_FOLDER}u-boot.bin
  cp ${FIP_BUILD_FOLDER}u-boot.bin.encrypt ${BUILD_FOLDER}u-boot.bin.encrypt
  cp ${FIP_BUILD_FOLDER}u-boot.bin.encrypt.efuse ${BUILD_FOLDER}u-boot.bin.encrypt.efuse
  cp ${FIP_BUILD_FOLDER}u-boot.bin.encrypt.sd.bin ${BUILD_FOLDER}u-boot.bin.encrypt.sd.bin
  cp ${FIP_BUILD_FOLDER}u-boot.bin.encrypt.usb.bl2 ${BUILD_FOLDER}u-boot.bin.encrypt.usb.bl2
  cp ${FIP_BUILD_FOLDER}u-boot.bin.encrypt.usb.tpl ${BUILD_FOLDER}u-boot.bin.encrypt.usb.tpl
  cp ${FIP_BUILD_FOLDER}u-boot.bin.sd.bin ${BUILD_FOLDER}u-boot.bin.sd.bin
  cp ${FIP_BUILD_FOLDER}u-boot.bin.usb.bl2 ${BUILD_FOLDER}u-boot.bin.usb.bl2
  cp ${FIP_BUILD_FOLDER}u-boot.bin.usb.tpl ${BUILD_FOLDER}u-boot.bin.usb.tpl

  if [ "y" == "${CONFIG_AML_CRYPTO_IMG}" ]; then
    cp ${FIP_BUILD_FOLDER}boot.img.encrypt ${BUILD_FOLDER}boot.img.encrypt
  fi
}

function build_fip() {
  local BLX_EXT=".bin"
  mkdir -p ${FIP_BUILD_FOLDER}

  fix_blx \
    ${FIP_BUILD_FOLDER}bl30.bin \
    ${FIP_BUILD_FOLDER}zero_tmp \
    ${FIP_BUILD_FOLDER}bl30_zero.bin \
    ${FIP_BUILD_FOLDER}bl301.bin \
    ${FIP_BUILD_FOLDER}bl301_zero.bin \
    ${FIP_BUILD_FOLDER}bl30_new.bin \
    bl30

  # acs_tool process ddr timing and configurable parameters
  python ${FIP_FOLDER}acs_tool.pyc ${FIP_BUILD_FOLDER}bl2.bin ${FIP_BUILD_FOLDER}bl2_acs.bin ${FIP_BUILD_FOLDER}acs.bin 0

  # fix bl2/bl21
  fix_blx \
    ${FIP_BUILD_FOLDER}bl2_acs.bin \
    ${FIP_BUILD_FOLDER}zero_tmp \
    ${FIP_BUILD_FOLDER}bl2_zero.bin \
    ${FIP_BUILD_FOLDER}bl21.bin \
    ${FIP_BUILD_FOLDER}bl21_zero.bin \
    ${FIP_BUILD_FOLDER}bl2_new.bin \
    bl2

  if [ "y" == "${CONFIG_FIP_IMG_SUPPORT}" ]; then
    BLX_EXT=".img"
  fi

  # v2: bl30/bl301 merged since 2016.03.22
  FIP_ARGS="--bl30 ${FIP_BUILD_FOLDER}bl30_new.bin --bl31 ${FIP_BUILD_FOLDER}bl31${BLX_EXT} --bl32 ${FIP_BUILD_FOLDER}bl32${BLX_EXT} --bl33 ${FIP_BUILD_FOLDER}bl33.bin"

  # create fip.bin
  ./${FIP_FOLDER}fip_create ${FIP_ARGS} ${FIP_BUILD_FOLDER}fip.bin
  ./${FIP_FOLDER}fip_create --dump ${FIP_BUILD_FOLDER}fip.bin

  # build final bootloader
  cat ${FIP_BUILD_FOLDER}bl2_new.bin ${FIP_BUILD_FOLDER}fip.bin > ${FIP_BUILD_FOLDER}boot.bin
  dd if=/dev/zero of=${FIP_BUILD_FOLDER}zero_512 bs=1 count=512

  # secure boot
  if [ "gxl" == ${CUR_SOC} ] || [ "txl" == ${CUR_SOC} ]; then
    ./${FIP_FOLDER}${CUR_SOC}/aml_encrypt_${CUR_SOC} --bl3enc  --input ${FIP_BUILD_FOLDER}bl30_new.bin
    ./${FIP_FOLDER}${CUR_SOC}/aml_encrypt_${CUR_SOC} --bl3enc  --input ${FIP_BUILD_FOLDER}bl31.bin
    ./${FIP_FOLDER}${CUR_SOC}/aml_encrypt_${CUR_SOC} --bl3enc  --input ${FIP_BUILD_FOLDER}bl33.bin
    ./${FIP_FOLDER}${CUR_SOC}/aml_encrypt_${CUR_SOC} --bl2sig  --input ${FIP_BUILD_FOLDER}bl2_new.bin   --output ${FIP_BUILD_FOLDER}bl2.n.bin.sig
    ./${FIP_FOLDER}${CUR_SOC}/aml_encrypt_${CUR_SOC} --bootmk  --output ${FIP_BUILD_FOLDER}u-boot.bin \
    --bl2   ${FIP_BUILD_FOLDER}bl2.n.bin.sig  --bl30  ${FIP_BUILD_FOLDER}bl30_new.bin.enc  \
    --bl31  ${FIP_BUILD_FOLDER}bl31.bin.enc --bl33  ${FIP_BUILD_FOLDER}bl33.bin.enc
  else
    ./${FIP_FOLDER}${CUR_SOC}/aml_encrypt_${CUR_SOC} --bootsig --input ${FIP_BUILD_FOLDER}boot.bin --output ${FIP_BUILD_FOLDER}u-boot.bin
  fi

  if [ "y" == "${CONFIG_AML_CRYPTO_UBOOT}" ]; then
    if [ "gxl" == ${CUR_SOC} ] || [ "txl" == ${CUR_SOC} ]; then
      ./${FIP_FOLDER}${CUR_SOC}/aml_encrypt_${CUR_SOC} --efsgen --amluserkey ${BLX_SRC_FOLDER[4]}/${BOARD_DIR}/aml-user-key.sig \
        --output ${FIP_BUILD_FOLDER}/u-boot.bin.encrypt.efuse
    fi
    ./${FIP_FOLDER}${CUR_SOC}/aml_encrypt_${CUR_SOC} --bootsig --input ${FIP_BUILD_FOLDER}/u-boot.bin --amluserkey ${BLX_SRC_FOLDER[4]}/${BOARD_DIR}/aml-user-key.sig \
      --aeskey enable --output ${FIP_BUILD_FOLDER}/u-boot.bin.encrypt
  fi

  if [ "y" == "${CONFIG_AML_CRYPTO_IMG}" ]; then
    # boot.img put in fip/ folder, todo
    ./${FIP_FOLDER}${CUR_SOC}/aml_encrypt_${CUR_SOC} --imgsig --input ${BLX_SRC_FOLDER[4]}/${BOARD_DIR}/boot.img --amluserkey ${BLX_SRC_FOLDER[4]}/${BOARD_DIR}/aml-user-key.sig --output ${FIP_BUILD_FOLDER}/boot.img.encrypt
  fi

  cat ${FIP_BUILD_FOLDER}zero_512 ${FIP_BUILD_FOLDER}u-boot.bin > ${FIP_BUILD_FOLDER}u-boot.bin.sd.bin
  if [ "y" == "${CONFIG_AML_CRYPTO_UBOOT}" ]; then
    cat ${FIP_BUILD_FOLDER}zero_512 ${FIP_BUILD_FOLDER}u-boot.bin.encrypt > ${FIP_BUILD_FOLDER}u-boot.bin.encrypt.sd.bin
  fi

  copy_bootloader

  echo "Bootloader build done!"
  return
}

function update_bin_path() {
  dbg "Update BIN_PATH[$1]=$2"
  BIN_PATH[$1]=$2
}

function clean() {
  echo "Clean up"
  cd ${BLX_SRC_FOLDER[4]}
  make distclean
  cd ${MAIN_FOLDER}
  rm ${FIP_BUILD_FOLDER} -rf
  rm ${BUILD_FOLDER} -rf
  return
}

function build() {
  clean
  get_versions
  pre_build_uboot $@
  build_uboot
  build_blx $@
  build_fip $@
}

function usage() {
  cat << EOF
  Usage:
    $(basename $0) --help

    build script.
    bl[x].bin priority:
    1. uboot will use binaries under ${BLX_BIN_FOLDER[@]}... folder by default, blx version specified in xml file.
    2. if you wanna use your own bl[x].bin, specify path by "--bl[x] path" parameter
    3. if you want update bl[x].bin by source code, please add "--update-bl[x]" parameter

    command list:

    1. build default uboot:
        ./$(basename $0) [config_name]
      eg:
        ./$(basename $0) gxb_p200_v1

    2. build uboot with specified blx.bin
        ./$(basename $0) [config_name] --bl[x] "bl[x].bin path"
      eg:
        ./$(basename $0) gxb_p200_v1 --bl2 fip/bl2.bin --bl30 fip/bl30.bin
      remark: this cmd will build uboot with specified bl2.bin, bl30.bin

    3. build uboot with blx source code
        ./$(basename $0) [config_name] --update-bl[x]
      eg:
        ./$(basename $0) gxb_p200_v1 --update-bl31 --update-bl2
      remark: this cmd will build uboot with bl31/bl2 source code

      usable configs:
`uboot_config_list`

EOF
  exit 1
}

function parser() {
  local i=0
  local argv=()
  for arg in "$@" ; do
    argv[$i]="$arg"
    i=$((i + 1))
  done
  i=0
  while [ $i -lt $# ]; do
    arg="${argv[$i]}"
    i=$((i + 1)) # must pleace here
    case "$arg" in
      -h|--help|help)
        usage
        return ;;
      --bl2)
        update_bin_path 0 "${argv[@]:$((i))}"
        continue ;;
      --bl30)
        update_bin_path 1 "${argv[@]:$((i))}"
        continue ;;
      --bl31)
        update_bin_path 2 "${argv[@]:$((i))}"
        continue ;;
      --bl32)
        update_bin_path 3 "${argv[@]:$((i))}"
        continue ;;
      --update-bl2)
        update_bin_path 0 "source"
        continue ;;
      --update-bl30)
        update_bin_path 1 "source"
        continue ;;
      --update-bl31)
        update_bin_path 2 "source"
        continue ;;
      --update-bl32)
        update_bin_path 3 "source"
        continue ;;
      clean|distclean|-distclean|--distclean)
        clean
        return ;;
      *)
    esac
  done
  build $@
}

function main() {
  if [ -z $1 ]
  then
    usage
    return
  fi

  MAIN_FOLDER=`pwd`
  parser $@
}

main $@ # parse all paras to function
