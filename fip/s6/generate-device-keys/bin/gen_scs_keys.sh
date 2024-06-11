#!/bin/bash -e

# Copyright (c) 2020 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.

#set -x
version=3.0
dbg_trace=1

EXEC_BASEDIR=$(dirname $(readlink -f $0))
ACPU_IMAGETOOL=
OPENSSL=${EXEC_BASEDIR}/../../bin/openssl-dilithium

declare -A sig_scheme_list
sig_scheme_list["rsa"]="rsa"
sig_scheme_list["mldsa"]="mldsa"
sig_scheme_list["rsa-mldsa"]="rsa-mldsa"

declare -A rsa_size_list
rsa_size_list["2048"]="2048"
rsa_size_list["4096"]="4096"

declare -A ml_dsa_level_list
ml_dsa_level_list["2"]="2"
ml_dsa_level_list["3"]="3"
ml_dsa_level_list["5"]="5"

declare -A ml_dsa_version_list
ml_dsa_version_list["draft1"]="draft1"
#ml_dsa_version["final"]="final"

declare -A stage_list
stage_list["root"]="root"
stage_list["boot-blobs"]="boot-blobs"
stage_list["fip"]="fip"
stage_list["fw"]="fw"
stage_list["ta"]="ta"

declare -A prefix_name_list
prefix_name_list["cs"]="cs-"
prefix_name_list["dv"]="dv-"
prefix_name_list["none"]=""

#TODO: To support DV needs to generate bl30 and bl33 FIP keys (based on dv vs cs prefix selection)

trace ()
{
    if [ ${dbg_trace} -ne 0 ]; then
    #echo ">>> $@" > /dev/null
    echo ">>> $@"
    fi
}

check_dir() {
    if [ ! -d "$1" ]; then echo "Error: directory \""$1"\" does NOT exist"; usage ; fi
}

rsa_sig() {
    if [ $is_rsa -eq 1 ]; then
    local chain_num=$1
    local path=$2
    local files=$3
    local payload=$4
    local ops=$5

    local test_vector_file="$path/test-payload-${payload}.bin"

    if [ $ops == "verify" ]; then
        echo "Verifying $chain_num ${rsa_algo_name^^} key test payload signature ..."
    else
        echo "Generating $chain_num ${rsa_algo_name^^} key test payload signature ..."
    fi

    if [ ! -f $test_vector_file ]; then
        if [ $ops == "verify" ]; then
         echo "No test payload file found"
            exit -1
        else
            trace "Creating dummy test payload $test_vector_file"
            dd if=/dev/random of=$test_vector_file bs=1024 count=2 iflag=fullblock
        fi
    fi

    # Sign a dummy payload with openssl 3.0.2
    #openssl pkeyutl -sign -rawin -in <dummy-payload.bin> -inkey <private-key.pem> -digest sha256 -out <pss.sha256.sig> -pkeyopt rsa_padding_mode:pss -pkeyopt rsa_pss_saltlen:digest

    # Verify signature using public key with openssl 3.0.2
    #openssl pkeyutl -verify -rawin -in <dummy-payload.bin> -sigfile <pss.sha256.sig> -pubin -inkey <public-key.pem> -digest sha256 -pkeyopt rsa_padding_mode:pss -pkeyopt rsa_pss_saltlen:digest

    for f in $files
    do
        if [ $ops == "verify" ]; then
            trace "openssl pkeyutl -verify -rawin -in $test_vector_file -sigfile $path/test-payload-${payload}-$f-pub.sig -pubin -inkey $path/$f-pub.pem -digest sha256 -pkeyopt rsa_padding_mode:pss -pkeyopt rsa_pss_saltlen:digest"
            openssl pkeyutl -verify -rawin -in $test_vector_file -sigfile $path/test-payload-${payload}-$f-pub.sig -pubin -inkey $path/$f-pub.pem -digest sha256 -pkeyopt rsa_padding_mode:pss -pkeyopt rsa_pss_saltlen:digest
        else
            trace "openssl pkeyutl -sign -rawin -in $test_vector_file -inkey $path/$f-priv.pem -digest sha256 -out $path/test-payload-${payload}-$f-pub.sig -pkeyopt rsa_padding_mode:pss -pkeyopt rsa_pss_saltlen:digest"
            openssl pkeyutl -sign -rawin -in $test_vector_file -inkey $path/$f-priv.pem -digest sha256 -out $path/test-payload-${payload}-$f-pub.sig -pkeyopt rsa_padding_mode:pss -pkeyopt rsa_pss_saltlen:digest
        fi
    done
    fi
}

rsa_gen() {
    if [ $is_rsa -eq 1 ]; then
    local chain_num=$1
    local path=$2
    local files=$3
    local size=$4

    echo "Generating trust chain $chain_num ${rsa_algo_name^^}-${size} key ..."

    for f in $files
    do
        local kpriv="$path/$f-priv.pem"
        local kpub="$path/$f-pub.pem"
        openssl genrsa -out $kpriv $size
        echo $kpriv
        echo $kpub
        openssl rsa -in $kpriv -outform PEM -pubout -out $kpub
        #openssl rsa -noout -text -inform PEM -in $kpub -pubin
    done
    fi
}

ml_dsa_sig() {
    if [ $is_ml_dsa -eq 1 ]; then
    local chain_num=$1
    local path=$2
    local files=$3
    local payload=$4
    local ops=$5

    local test_vector_file="$path/test-payload-${payload}.bin"

    if [ $ops == "verify" ]; then
        echo "Verifying $chain_num ${ml_dsa_algo_name^^} key test payload signature ..."
    else
        echo "Generating $chain_num ${ml_dsa_algo_name^^} key test payload signature ..."
    fi

    if [ ! -f $test_vector_file ]; then
        if [ $ops == "verify" ]; then
            echo "No test payload file found"
            exit -1
        else
            trace "Creating dummy test payload $test_vector_file"
            dd if=/dev/random of=$test_vector_file bs=1024 count=2 iflag=fullblock
        fi
    fi

    # Sign a dummy payload with private openssl-dilithium 1.1.1u build
    #openssl dgst -sha3-512 -sign <private-key.pem> -keyform pem -out <ml-dsa.sig> <dummy-payload.bin>

    # Verify signature using public key with private openssl-dilithium 1.1.1u build
    #openssl dgst -sha3-512 -verify <public-key.pem> -keyform pem -signature <ml-dsa.sig> <dummy-payload.bin>

    for f in $files
    do
        if [ $ops == "verify" ]; then
            trace "${OPENSSL} dgst -sha3-512 -verify $path/$f-pub.pem -keyform pem -signature $path/test-payload-${payload}-$f-pub.sig $test_vector_file"
            ${OPENSSL} dgst -sha3-512 -verify $path/$f-pub.pem -keyform pem -signature $path/test-payload-${payload}-$f-pub.sig $test_vector_file
        else
            trace "${OPENSSL} dgst -sha3-512 -sign $path/$f-priv.pem -keyform pem -out $path/test-payload-${payload}-$f-pub.sig $test_vector_file"
            ${OPENSSL} dgst -sha3-512 -sign $path/$f-priv.pem -keyform pem -out $path/test-payload-${payload}-$f-pub.sig $test_vector_file
        fi
    done
    fi
}

ml_dsa_gen() {
    if [ $is_ml_dsa -eq 1 ]; then
    local chain_num=$1
    local path=$2
    local files=$3
    local size=$4

    echo "Generating trust chain $chain_num ${ml_dsa_algo_name^^}-${size} key ..."

    for f in $files
    do
        local kpriv="$path/$f-priv.pem"
        local kpub="$path/$f-pub.pem"
        ${OPENSSL} genpkey -algorithm dilithium${size} -outform PEM -out $kpriv
        echo $kpriv
        echo $kpub
        ${OPENSSL} pkey -in $kpriv -outform PEM -pubout -out $kpub
    done
    fi
}

key_link() {
    local chain_num=$1
    local path=$2
    local src=$3
    local files=$4

    echo "Linking trust chain $chain_num key ..."

    local kpriv_src="$src-priv.pem"
    local kpub_src="$src-pub.pem"
    echo $kpriv_src
    echo $kpub_src

    pushd $path
    for f in $files
    do
        local kpriv="$f-priv.pem"
        local kpub="$f-pub.pem"

        echo $kpriv
        echo $kpub

        ln -s $kpriv_src $kpriv
        ln -s $kpub_src $kpub

        ls -l $kpriv
        ls -l $kpub
        #openssl pkey -noout -text -inform PEM -in $kpub -pubin
    done
    popd
}

ek_link() {
    local chain_num=$1
    local path=$2
    local src=$3
    local files=$4

    echo "Linking trust chain $chain_num EKs ..."

    local file_src=$src
    echo $file_src

    pushd $path
    for f in $files
    do
        local file="$f"
        echo $file

        ln -s $file_src $file

        ls -l $file
        #dd if=/dev/random of=$file iflag=fullblock bs=64 count=1
        #xxd -p -c16 $file
    done
    popd
}

ek_gen() {
    local chain_num=$1
    local path=$2
    local files=$3

    echo "Generating trust chain $chain_num EKs ..."

    for f in $files
    do
        local file="$path/$f"
        echo $file
        dd if=/dev/random of=$file iflag=fullblock bs=64 count=1
        #xxd -p -c16 $file
    done
}

nonce_link() {
    local chain_num=$1
    local path=$2
    local src=$3
    local files=$4

    echo "Linking trust chain $chain_num NONCE ..."

    local file_src=$src
    echo $file_src

    pushd $path
    for f in $files
    do
        local file="$f"
        echo $file

        ln -s $file_src $file

        ls -l $file
        #dd if=/dev/random of=$file iflag=fullblock bs=16 count=1
        #xxd -p -c16 $file
    done
    popd
}

nonce_gen() {
    local chain_num=$1
    local path=$2
    local files=$3

    echo "Generating trust chain $chain_num NONCE ..."

    for f in $files
    do
        local file="$path/$f"
        echo $file
        dd if=/dev/random of=$file iflag=fullblock bs=16 count=1
        #xxd -p -c16 $file
    done
}

usage() {
    cat << EOF
Usage: $(basename $0) --help | --version

        Generate Amlogic Secure Chipset Startup (SCS) keys

        $(basename $0)
        --key-dir <key-dir> \\
        --stage [root | boot-blobs] \\
        {--sig-scheme [rsa | mldsa | rsa-mldsa]} \\
        {--prefix [cs | dv]} \\
        {--rsa-size [2048 | 4096]} \\
        {--ml-dsa-level [2 | 3 | 5]} \\
        {--ml-dsa-version [draft1]} \\
        {--gen-sig} \\
        {--verify-sig} \\
        {--link-gen-file [0 | 1]} \\
        {--link-lvl3-to-lvl2-file [0 | 1]} \\
        {--project <project-name>}
EOF
    exit 1
}

PREFIX="cs-"
key_dir=""
part=""
rsa_size=""
stage=""
fw_type="aucpu vdec"
gen_sig=0
verify_sig=0
link_gen_file=0
link_lvl3_to_lvl2_file=0
sig_scheme=""
sig_scheme_version=""
prefix=""

# Change root trust chain name from "rootrsa" to "trustchain"
trustchain_name="trustchain"

is_rsa=1
is_ml_dsa=0
is_hybrid=0

rsa_algo_name="rsa"
ml_dsa_level=""
ml_dsa_algo_name="mldsa"
ml_dsa_version=""

parse_main() {
    local i=0
    local argv=()
    for arg in "$@" ; do
        argv[$i]="$arg"
        i=$((i + 1))
    done

    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            -h|--help)
                usage
                break
                ;;
            -v|--version)
                echo "Version $version";
                exit 0
                ;;
            --key-dir)
                key_dir="${argv[$i]}"
                check_dir "${key_dir}"
                ;;
            --project)
                part="${argv[$i]}"
                ;;
                # Backward compatible
                --size)
                rsa_size="${argv[$i]}"
                ;;
            --rsa-size)
                rsa_size="${argv[$i]}"
                ;;
            --stage)
                stage="${argv[$i]}"
                ;;
            --fw-type)
                fw_type="${argv[$i]}"
                ;;
            --gen-sig)
                gen_sig=1
                i=$((i - 1))
                ;;
            --verify-sig)
                verify_sig=1
                i=$((i - 1))
                ;;
            --link-gen-file)
                link_gen_file="${argv[$i]}"
                ;;
            --link-lvl3-to-lvl2-file)
                link_lvl3_to_lvl2_file="${argv[$i]}"
                ;;
            --sig-scheme)
                sig_scheme="${argv[$i]}"
                ;;
            --prefix)
                prefix_name="${argv[$i]}"
                ;;
            --ml-dsa-level)
                ml_dsa_level="${argv[$i]}"
                ;;
            --ml-dsa-version)
                ml_dsa_version="${argv[$i]}"
                ;;
            *)
                echo "Unknown option $arg";
                usage
                ;;
        esac
        i=$((i + 1))
    done
}

parse_main "$@"

trace "       key-dir $key_dir"
trace "       project $part"
trace "      rsa-size $rsa_size"
trace "  ml-dsa-level $ml_dsa_level"
trace "ml-dsa-version $ml_dsa_version"
trace "         stage $stage"
trace "       fw-type $fw_type"
trace "       gen-sig $gen_sig"
trace "    verify-sig $verify_sig"
trace " link-gen-file $link_gen_file"
trace "    sig-scheme $sig_scheme"
trace "        prefix $prefix_name"
trace "       OPENSSL $OPENSSL"
trace " link-lvl3-to-lvl2-file $link_lvl3_to_lvl2_file"

if [ -z "$key_dir" ]; then
    usage
fi

if [ -z "$rsa_size" ]; then
    rsa_size=4096
fi

if [ -z "$ml_dsa_level" ]; then
    ml_dsa_level=3
fi

if [ -z "$sig_scheme" ]; then
    sig_scheme="rsa"
fi

if [ -z "$prefix_name" ]; then
    prefix_name="cs"
fi

if [ -z "$stage" ]; then
    usage
fi

tmp=${rsa_size_list[$rsa_size]}
if [ "$tmp" == "" ]; then
    echo "Error: Invalid RSA key size $rsa_size"
    usage
fi

#
# Only root and boot-blobs supports ML-DSA as of now
# Keep FIP/FW/TA for future usage
#
tmp=${stage_list[$stage]}
if [ "$tmp" == "" ]; then
    echo "Error: Invalid stage $stage"
    usage
fi

PREFIX=${prefix_name_list[$prefix_name]}
if [ "$PREFIX" == "" ] && [ "$prefix_name" != "none" ]; then
    echo "Error: Invalid prefix $prefix_name"
    usage
fi

tmp=${sig_scheme_list[$sig_scheme]}
if [ "${tmp}" == "" ]; then
    echo "Error: Invalid signature scheme $sig_scheme"
    usage
fi

sig_scheme=$tmp
if [ ${sig_scheme} == "mldsa" ] || [ ${sig_scheme} == "rsa-mldsa" ]; then
    if [ -z "$ml_dsa_version" ]; then
        echo "Error: Missing ML-DSA version"
        usage
    fi

    tmp=${ml_dsa_level_list[$ml_dsa_level]}
    if [ "$tmp" == "" ]; then
        echo "Error: Invalid ML-DSA key level $ml_dsa_level"
        usage
    fi
    
    tmp=${ml_dsa_version_list[$ml_dsa_version]}
    if [ "$tmp" == "" ]; then
        echo "Error: Invalid ML-DSA version $ml_dsa_version"
        usage
    fi
fi

if [ ${sig_scheme} == "rsa" ]; then
    is_rsa=1
    is_ml_dsa=0
    is_hybrid=0
    sig_scheme_version=${sig_scheme}
fi
if [ ${sig_scheme} == "mldsa" ]; then
    is_rsa=0
    is_ml_dsa=1
    is_hybrid=0
    if [ "${ml_dsa_version}" != "final" ]; then
        sig_scheme_version=${sig_scheme}-${ml_dsa_version}
        ml_dsa_algo_name=${ml_dsa_algo_name}-${ml_dsa_version}
    fi
fi
if [ ${sig_scheme} == "rsa-mldsa" ]; then
    is_rsa=1
    is_ml_dsa=1
    is_hybrid=1
    if [ "${ml_dsa_version}" != "final" ]; then
        sig_scheme_version=${sig_scheme}-${ml_dsa_version}
        ml_dsa_algo_name=${ml_dsa_algo_name}-${ml_dsa_version}
    fi
fi

root_key_path=${key_dir}/root/${sig_scheme}
boot_blobs_key_root=${key_dir}/boot-blobs/${sig_scheme}
fip_key_root=${key_dir}/fip/${sig_scheme}
boot_blobs_key_rel_to_fip_path=../../boot-blobs/${sig_scheme}
fw_key_root=${key_dir}/firmware/rsa
ta_key_root=${key_dir}/ta/rsa

if [ ! -z "$part" ]; then
    root_key_path=${root_key_path}/$part
    boot_blobs_key_root=${boot_blobs_key_root}/$part
    fip_key_root=${fip_key_root}/$part
    boot_blobs_key_rel_to_fip_path=../../../boot-blobs/${sig_scheme}/$part
    fw_key_root=${fw_key_root}/$part
    ta_key_root=${ta_key_root}/$part
fi

trace "            PREFIX $PREFIX"
trace "          rsa-size $rsa_size"
trace "      ml-dsa-level $ml_dsa_level"
trace "    ml-dsa-version $ml_dsa_version"
trace "        sig-scheme $sig_scheme"
trace "sig-scheme-version $sig_scheme_version"
trace "            is_rsa $is_rsa"
trace "         is_ml_dsa $is_ml_dsa"
trace "         is_hybrid $is_hybrid"

if [ $gen_sig -eq 1 ]; then
    if [ $stage == "root" ]; then
        rsa_sig "Root" "${root_key_path}/key" "${PREFIX}root${rsa_algo_name}-0 ${PREFIX}root${rsa_algo_name}-1 ${PREFIX}root${rsa_algo_name}-2 ${PREFIX}root${rsa_algo_name}-3" "root" "sign"
        ml_dsa_sig "Root" "${root_key_path}/key" "${PREFIX}root${ml_dsa_algo_name}-0 ${PREFIX}root${ml_dsa_algo_name}-1 ${PREFIX}root${ml_dsa_algo_name}-2 ${PREFIX}root${ml_dsa_algo_name}-3" "root" "sign"
    fi
    
    if [ $stage == "boot-blobs" ]; then
        for i in 0 1 2 3
        do
            boot_blobs_key_path=${boot_blobs_key_root}/${trustchain_name}-${i}
            trace " boot_blobs_key_path ${boot_blobs_key_path}"
            
            rsa_sig "${PREFIX}lvl1/2-$i" "${boot_blobs_key_path}/key" "${PREFIX}level-1-${rsa_algo_name} ${PREFIX}level-2-${rsa_algo_name}" "boot-blobs-$i" "sign"
            ml_dsa_sig "${PREFIX}lvl1/2-$i" "${boot_blobs_key_path}/key" "${PREFIX}level-1-${ml_dsa_algo_name} ${PREFIX}level-2-${ml_dsa_algo_name}" "boot-blobs-$i" "sign"
        done
    fi

    if [ $stage == "fip" ]; then
        for i in 0 1 2 3
        do
            fip_key_path=${fip_key_root}/${trustchain_name}-${i}
            trace " fip_key_path ${fip_key_path}"

            rsa_sig "${PREFIX}fip-$i" "${fip_key_path}/key" "${PREFIX}bl31-level-3-${rsa_algo_name} ${PREFIX}bl32-level-3-${rsa_algo_name} ${PREFIX}bl40-level-3-${rsa_algo_name}" "fip-$i" "sign"
            ml_dsa_sig "${PREFIX}fip-$i" "${fip_key_path}/key" "${PREFIX}bl31-level-3-${ml_dsa_algo_name} ${PREFIX}bl32-level-3-${ml_dsa_algo_name} ${PREFIX}bl40-level-3-${ml_dsa_algo_name}" "fip-$i" "sign"
        done
    fi

    if [ $stage == "fw" ]; then
        # HACK: Force is_dsa to 1 as no PQC support for FW
        is_rsa=1
        for i in ${fw_type}
        do
            fw_key_path=${fw_key_root}/${i}
            trace " fw_key_path ${fw_key_path}"

            rsa_sig $i "${fw_key_path}/key" "${PREFIX}fw-${i}-${rsa_algo_name}" "fw-$i" "sign"
            # No ML-DSA for FW
            #ml_dsa_sig $i "${fw_key_path}/key" "${PREFIX}fw-${i}-${ml_dsa_algo_name}" "fw-$i" "sign"
        done
    fi

    if [ $stage == "ta" ]; then
        # HACK: Force is_dsa to 1 as no PQC support for TA
        is_rsa=1
        for i in rsk
        do
            ta_key_path=${ta_key_root}/${i}
            trace " ta_key_path ${ta_key_path}"

            rsa_sig $i "${ta_key_path}/key" "${PREFIX}ta-${i}-${rsa_algo_name}" "ta-$i" "sign"
            # No ML-DSA for TA
            #ml_dsa_sig $i "${ta_key_path}/key" "${PREFIX}ta-${i}-${ml_dsa_algo_name}" "ta-$i" "sign"
        done
    fi

    #
    # HACK: Set stage to un-supported name to skip generation
    #
    stage="skip"
fi

if [ $verify_sig -eq 1 ]; then
    if [ $stage == "root" ]; then
        rsa_sig "Root" "${root_key_path}/key" "${PREFIX}root${rsa_algo_name}-0 ${PREFIX}root${rsa_algo_name}-1 ${PREFIX}root${rsa_algo_name}-2 ${PREFIX}root${rsa_algo_name}-3" "root" "verify"
        ml_dsa_sig "Root" "${root_key_path}/key" "${PREFIX}root${ml_dsa_algo_name}-0 ${PREFIX}root${ml_dsa_algo_name}-1 ${PREFIX}root${ml_dsa_algo_name}-2 ${PREFIX}root${ml_dsa_algo_name}-3" "root" "verify"
    fi

    if [ $stage == "boot-blobs" ]; then
        for i in 0 1 2 3
        do
            boot_blobs_key_path=${boot_blobs_key_root}/${trustchain_name}-${i}
            trace " boot_blobs_key_path ${boot_blobs_key_path}"

            rsa_sig "${PREFIX}lvl1/2-$i" "${boot_blobs_key_path}/key" "${PREFIX}level-1-${rsa_algo_name} ${PREFIX}level-2-${rsa_algo_name}" "boot-blobs-$i" "verify"
            ml_dsa_sig "${PREFIX}lvl1/2-$i" "${boot_blobs_key_path}/key" "${PREFIX}level-1-${ml_dsa_algo_name} ${PREFIX}level-2-${ml_dsa_algo_name}" "boot-blobs-$i" "verify"
        done
    fi

    if [ $stage == "fip" ]; then
        for i in 0 1 2 3
        do
            fip_key_path=${fip_key_root}/${trustchain_name}-${i}
            trace " fip_key_path ${fip_key_path}"

            rsa_sig "${PREFIX}fip-$i" "${fip_key_path}/key" "${PREFIX}bl31-level-3-${rsa_algo_name} ${PREFIX}bl32-level-3-${rsa_algo_name} ${PREFIX}bl40-level-3-${rsa_algo_name}" "fip-$i" "verify"
            ml_dsa_sig "${PREFIX}fip-$i" "${fip_key_path}/key" "${PREFIX}bl31-level-3-${ml_dsa_algo_name} ${PREFIX}bl32-level-3-${ml_dsa_algo_name} ${PREFIX}bl40-level-3-${ml_dsa_algo_name}" "fip-$i" "verify"
        done
    fi

    if [ $stage == "fw" ]; then
        # HACK: Force is_dsa to 1 as no PQC support for FW
        is_rsa=1
        for i in ${fw_type}
        do
            fw_key_path=${fw_key_root}/${i}
            trace " fw_key_path ${fw_key_path}"

            rsa_sig $i "${fw_key_path}/key" "${PREFIX}fw-${i}-${rsa_algo_name}" "fw-$i" "verify"
            # No ML-DSA for FW
            #ml_dsa_sig $i "${fw_key_path}/key" "${PREFIX}fw-${i}-${ml_dsa_algo_name}" "fw-$i" "verify"
        done
    fi

    if [ $stage == "ta" ]; then
        # HACK: Force is_dsa to 1 as no PQC support for TA
        is_rsa=1
        for i in rsk
        do
            ta_key_path=${ta_key_root}/${i}
            trace " ta_key_path ${ta_key_path}"
            
            rsa_sig $i "${ta_key_path}/key" "${PREFIX}ta-${i}-${rsa_algo_name}" "ta-$i" "verify"
            # No ML-DSA for TA
            #ml_dsa_sig $i "${ta_key_path}/key" "${PREFIX}ta-${i}-${ml_dsa_algo_name}" "ta-$i" "verify"
        done
    fi

    #
    # HACK: Set stage to un-supported name to skip generation
    #
    stage="skip"
fi

if [ $stage == "root" ]; then
    trace " root_key_path ${root_key_path}"
    mkdir -p ${root_key_path}/key
    mkdir -p ${root_key_path}/epk
    mkdir -p ${root_key_path}/nonce

    echo "Generate Root keys"

    rsa_gen "Root" "$root_key_path/key" "${PREFIX}root${rsa_algo_name}-0 ${PREFIX}root${rsa_algo_name}-1 ${PREFIX}root${rsa_algo_name}-2 ${PREFIX}root${rsa_algo_name}-3" $rsa_size
    ml_dsa_gen "Root" "$root_key_path/key" "${PREFIX}root${ml_dsa_algo_name}-0 ${PREFIX}root${ml_dsa_algo_name}-1 ${PREFIX}root${ml_dsa_algo_name}-2 ${PREFIX}root${ml_dsa_algo_name}-3" $ml_dsa_level

    ek_gen "Root" "$root_key_path/epk" "${PREFIX}rootcert-epks.bin"
    nonce_gen "Root" "$root_key_path/nonce" "${PREFIX}rootkey-0-nonce.bin ${PREFIX}rootkey-1-nonce.bin ${PREFIX}rootkey-2-nonce.bin ${PREFIX}rootkey-3-nonce.bin"
fi

if [ $stage == "boot-blobs" ]; then
    trace " boot_blobs_key_root ${boot_blobs_key_root}"
    mkdir -p ${boot_blobs_key_root}

    for i in 0 1 2 3
    do
        boot_blobs_key_path=${boot_blobs_key_root}/${trustchain_name}-${i}
        trace " boot_blobs_key_path ${boot_blobs_key_path}"

        mkdir -p ${boot_blobs_key_path}/key
        mkdir -p ${boot_blobs_key_path}/epk
        mkdir -p ${boot_blobs_key_path}/nonce

        if [ $link_gen_file -eq 1 ]; then
            echo "Generate & link $stage chain #$i key"
            rsa_gen $i "${boot_blobs_key_path}/key" "${PREFIX}level-1-2-${rsa_algo_name}" $rsa_size
            ml_dsa_gen $i "${boot_blobs_key_path}/key" "${PREFIX}level-1-2-${ml_dsa_algo_name}" $ml_dsa_level
            if [ $is_rsa -eq 1 ]; then
                key_link $i "${boot_blobs_key_path}/key" "${PREFIX}level-1-2-${rsa_algo_name}" "${PREFIX}level-1-${rsa_algo_name} ${PREFIX}level-2-${rsa_algo_name}"
            fi
            if [ $is_ml_dsa -eq 1 ]; then
                key_link $i "${boot_blobs_key_path}/key" "${PREFIX}level-1-2-${ml_dsa_algo_name}" "${PREFIX}level-1-${ml_dsa_algo_name} ${PREFIX}level-2-${ml_dsa_algo_name}"
            fi

            # TODO: Nonce and EK should be separated generated and linked here
            ek_gen $i "${boot_blobs_key_path}/epk" "${PREFIX}lvl-1-2-cert-epks.bin"
            nonce_gen $i "${boot_blobs_key_path}/nonce" "${PREFIX}lvl-1-2-key-nonce.bin"
            ek_link $i "${boot_blobs_key_path}/epk" "${PREFIX}lvl-1-2-cert-epks.bin" "${PREFIX}lvl1cert-epks.bin ${PREFIX}lvl2cert-epks.bin"
            nonce_link $i "${boot_blobs_key_path}/nonce" "${PREFIX}lvl-1-2-key-nonce.bin" "${PREFIX}lvl1key-nonce.bin ${PREFIX}lvl2key-nonce.bin"
        else
            echo "Generate $stage chain #$i key"
            rsa_gen $i "${boot_blobs_key_path}/key" "${PREFIX}level-1-${rsa_algo_name} ${PREFIX}level-2-${rsa_algo_name}" $rsa_size
            ml_dsa_gen $i "${boot_blobs_key_path}/key" "${PREFIX}level-1-${ml_dsa_algo_name} ${PREFIX}level-2-${ml_dsa_algo_name}" $ml_dsa_level

            # TODO: Nonce and EK should be separated generated and linked here
            ek_gen $i "${boot_blobs_key_path}/epk" "${PREFIX}lvl1cert-epks.bin ${PREFIX}lvl2cert-epks.bin"
            nonce_gen $i "${boot_blobs_key_path}/nonce" "${PREFIX}lvl1key-nonce.bin ${PREFIX}lvl2key-nonce.bin"
        fi
    done
fi

if [ $stage == "fip" ]; then
    trace " fip_key_root ${fip_key_root}"
    mkdir -p ${fip_key_root}

    for i in 0 1 2 3
    do
        fip_key_path=${fip_key_root}/${trustchain_name}-${i}
        trace " fip_key_path ${fip_key_path}"

        mkdir -p ${fip_key_path}/key
        mkdir -p ${fip_key_path}/epk
        mkdir -p ${fip_key_path}/nonce

        # Link level-3 keys to level-2 for compatibility
        if [ $link_gen_file -eq 1 ]; then
            echo "Generate & link ${stage^^} chain #$i key"
            if [ $is_ml_dsa -eq 1 ]; then
                echo "Error: No compact FIP header support for ML-DSA"
        exit 1
            fi

            rsa_gen $i "${fip_key_path}/key" "${PREFIX}bl3x-level-3-${rsa_algo_name}" $rsa_size
            ek_gen $i "${fip_key_path}/epk" "${PREFIX}bl3x-lvl3cert-epks.bin"
            nonce_gen $i "${fip_key_path}/nonce" "${PREFIX}bl3x-lvl3key-nonce.bin"

            key_link $i "${fip_key_path}/key" "${PREFIX}bl3x-level-3-${rsa_algo_name}" "${PREFIX}bl31-level-3-${rsa_algo_name} ${PREFIX}bl32-level-3-${rsa_algo_name} ${PREFIX}bl40-level-3-${rsa_algo_name}" $rsa_size
            ek_link $i "${fip_key_path}/epk" "${PREFIX}bl3x-lvl3cert-epks.bin" "${PREFIX}bl31-lvl3cert-epks.bin ${PREFIX}bl32-lvl3cert-epks.bin ${PREFIX}bl40-lvl3cert-epks.bin"
            nonce_link $i "${fip_key_path}/nonce" "${PREFIX}bl3x-lvl3key-nonce.bin" "${PREFIX}bl31-lvl3key-nonce.bin ${PREFIX}bl32-lvl3key-nonce.bin ${PREFIX}bl40-lvl3key-nonce.bin"
        elif [ $link_lvl3_to_lvl2_file -eq 1 ]; then
            # To keep compatibility of old script, create linked Level-3 key to Level-2 key
            #boot_blobs_key_path=${boot_blobs_key_root}/{trustchain_name}-${i}
            boot_blobs_key_rel_path=../../${boot_blobs_key_rel_to_fip_path}/${trustchain_name}-${i}
            trace " boot_blobs_key_path ${boot_blobs_key_path}"

            echo "link ${stage^^} chain #$i to level-2 key"
            key_link $i "${fip_key_path}/key" "${boot_blobs_key_rel_path}/key/${PREFIX}level-2-${rsa_algo_name}" "${PREFIX}bl31-level-3-${rsa_algo_name} ${PREFIX}bl32-level-3-${rsa_algo_name} ${PREFIX}bl40-level-3-${rsa_algo_name}"
            if [ $is_ml_dsa -eq 1 ]; then
                key_link $i "${fip_key_path}/key" "${boot_blobs_key_rel_path}/key/${PREFIX}level-2-${ml_dsa_algo_name}" "${PREFIX}bl31-level-3-${ml_dsa_algo_name} ${PREFIX}bl32-level-3-${ml_dsa_algo_name} ${PREFIX}bl40-level-3-${ml_dsa_algo_name}"
            fi
            ek_link $i "${fip_key_path}/epk" "${boot_blobs_key_rel_path}/epk/${PREFIX}lvl2cert-epks.bin" "${PREFIX}bl31-lvl3cert-epks.bin ${PREFIX}bl32-lvl3cert-epks.bin ${PREFIX}bl40-lvl3cert-epks.bin"
            nonce_link $i "${fip_key_path}/nonce" "${boot_blobs_key_rel_path}/nonce/${PREFIX}lvl2key-nonce.bin" "${PREFIX}bl31-lvl3key-nonce.bin ${PREFIX}bl32-lvl3key-nonce.bin ${PREFIX}bl40-lvl3key-nonce.bin"

            if [ "$prefix_name" == "dv" ] || [ "$prefix_name" == "none" ]; then
                key_link $i "${fip_key_path}/key" "${boot_blobs_key_rel_path}/key/${PREFIX}level-2-${rsa_algo_name}" "${PREFIX}bl30-level-3-${rsa_algo_name} ${PREFIX}bl33-level-3-${rsa_algo_name} ${PREFIX}krnl-level-3-${rsa_algo_name}"
                if [ $is_ml_dsa -eq 1 ]; then
                    key_link $i "${fip_key_path}/key" "${boot_blobs_key_rel_path}/key/${PREFIX}level-2-${ml_dsa_algo_name}" "${PREFIX}bl30-level-3-${ml_dsa_algo_name} ${PREFIX}bl33-level-3-${ml_dsa_algo_name} ${PREFIX}krnl-level-3-${ml_dsa_algo_name}"
                fi
                ek_link $i "${fip_key_path}/epk" "${boot_blobs_key_rel_path}/epk/${PREFIX}lvl2cert-epks.bin" "${PREFIX}bl30-lvl3cert-epks.bin ${PREFIX}bl33-lvl3cert-epks.bin ${PREFIX}krnl-lvl3cert-epks.bin"
                nonce_link $i "${fip_key_path}/nonce" "${boot_blobs_key_rel_path}/nonce/${PREFIX}lvl2key-nonce.bin" "${PREFIX}bl30-lvl3key-nonce.bin ${PREFIX}bl33-lvl3key-nonce.bin ${PREFIX}krnl-lvl3key-nonce.bin"
            fi
        else
            echo "Generate ${stage^^} chain #$i key"
            rsa_gen $i "${fip_key_path}/key" "${PREFIX}bl31-level-3-${rsa_algo_name} ${PREFIX}bl32-level-3-${rsa_algo_name} ${PREFIX}bl40-level-3-${rsa_algo_name}" $rsa_size
            if [ $is_ml_dsa -eq 1 ]; then
                ml_dsa_gen $i "${fip_key_path}/key" "${PREFIX}bl31-level-3-${ml_dsa_algo_name} ${PREFIX}bl32-level-3-${ml_dsa_algo_name} ${PREFIX}bl40-level-3-${ml_dsa_algo_name}" $ml_dsa_level
            fi
            ek_gen $i "${fip_key_path}/epk" "${PREFIX}bl31-lvl3cert-epks.bin ${PREFIX}bl32-lvl3cert-epks.bin ${PREFIX}bl40-lvl3cert-epks.bin"
            nonce_gen $i "${fip_key_path}/nonce" "${PREFIX}bl31-lvl3key-nonce.bin ${PREFIX}bl32-lvl3key-nonce.bin ${PREFIX}bl40-lvl3key-nonce.bin"

            if [ "$prefix_name" == "dv" ] || [ "$prefix_name" == "none" ]; then
                rsa_gen $i "${fip_key_path}/key" "${PREFIX}bl30-level-3-${rsa_algo_name} ${PREFIX}bl33-level-3-${rsa_algo_name} ${PREFIX}krnl-level-3-${rsa_algo_name}" $rsa_size
                if [ $is_ml_dsa -eq 1 ]; then
                    ml_dsa_gen $i "${fip_key_path}/key" "${PREFIX}bl30-level-3-${ml_dsa_algo_name} ${PREFIX}bl33-level-3-${ml_dsa_algo_name} ${PREFIX}krnl-level-3-${ml_dsa_algo_name}" $ml_dsa_level
                fi
                ek_gen $i "${fip_key_path}/epk" "${PREFIX}bl30-lvl3cert-epks.bin ${PREFIX}bl33-lvl3cert-epks.bin ${PREFIX}krnl-lvl3cert-epks.bin"
                nonce_gen $i "${fip_key_path}/nonce" "${PREFIX}bl30-dvlvl3key-nonce.bin ${PREFIX}bl33-dvlvl3key-nonce.bin ${PREFIX}krnl-dvlvl3key-nonce.bin"
            fi
        fi
    done
fi

if [ $stage == "fw" ]; then
    trace " fw_key_root ${fw_key_root}"
    mkdir -p ${fw_key_root}

    # HACK: Force is_dsa to 1 in case of FIP as no PQC support for FW
    is_rsa=1

    # No FW ML-DSA support yet
    for i in ${fw_type}
    do
        fw_key_path=${fw_key_root}/${i}
        trace " fw_key_path ${fw_key_path}"

        mkdir -p ${fw_key_path}/key
        mkdir -p ${fw_key_path}/epk
        mkdir -p ${fw_key_path}/nonce

        echo "Generate FW ${i} key"
        rsa_gen $i "${fw_key_path}/key" "${PREFIX}fw-${i}-${rsa_algo_name}" $rsa_size
        ek_gen $i "${fw_key_path}/epk" "${PREFIX}fw-${i}-cert-epks.bin"
        nonce_gen $i "${fw_key_path}/nonce" "${PREFIX}fw-${i}-key-nonce.bin"
    done
fi

if [ $stage == "ta" ]; then
    trace " ta_key_root ${ta_key_root}"
    mkdir -p ${ta_key_root}

    # HACK: Force is_dsa to 1 in case of FIP as no PQC support for TA
    is_rsa=1

    # No TA ML-DSA support yet
    for i in rsk
    do
        ta_key_path=${ta_key_root}/${i}
        trace " ta_key_path ${ta_key_path}"

        mkdir -p ${ta_key_path}/key
        #mkdir -p $ta_key_path/epk
        #mkdir -p $ta_key_path/nonce

        echo "Generate TA ${i} key"
        rsa_gen $i "${ta_key_path}/key" "${PREFIX}ta-${i}-${rsa_algo_name}" $rsa_size
        #ek_gen $i "${ta_key_path}/epk" "${PREFIX}ta-${i}-cert-epks.bin"
        #nonce_gen $i "${ta_key_path}/nonce" "${PREFIX}ta-${i}-key-nonce.bin"
    done
fi
