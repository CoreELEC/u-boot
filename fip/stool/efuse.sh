#!/bin/bash

usage() {
    cat << EOF
Usage: $(basename $0) --help
       $(basename $0) --version
       $(basename $0) --generate-efuse-pattern \\
                      --soc [gxl | txlx | g12a | g12b ] \\
                      [--aml-key-path path-of-key]      \\
                      [--enable-sb false]               \\
                      [--enable-aes false]              \\
                      [--password-hash password.hash]   \\
                      [--enable-jtag-password false]    \\
                      [--enable-usb-password false]     \\
                      [--enable-anti-rollback false]    \\
                      -o pattern.efuse

EOF
    exit 1
}

amlkeypath=""
kwrap=""
wrlock_kwrap="false"
roothash=""
passwordhash=""
scanpasswordhash=""
userefusefile=""
aeskey=""
m4roothash=""
m4aeskey=""
enablesb="false"
enableaes="false"
enablejtagpassword="false"
enableusbpassword="false"
enablescanpassword="false"
enableantirollback="false"
disablebootusb="false"
disablebootspi="false"
disablebootsdcard="false"
disablebootnandemmc="false"
disablebootrecover="false"
disableprint="false"
disablejtag="false"
disablescanchain="false"
revokersk0="false"
revokersk1="false"
revokersk2="false"
revokersk3="false"
output=""
sigver=""
keyhashver=""
soc="unknown"
socrev="a"
opt_raw_otp_pattern="false"

generate_efuse_pattern() {
    local argv=("$@")
    local i=0
     # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
	#echo "i=$i arg=\"$arg\""
        i=$((i + 1))
	#echo "i=$i argv[$i]=${argv[$i]}"
        case "$arg" in
            --aml-key-path)
               amlkeypath="${argv[$i]}" ;;
            --kwrap)
                kwrap="${argv[$i]}" ;;
            --root-hash)
                roothash="${argv[$i]}" ;;
            --password-hash)
                passwordhash="${argv[$i]}" ;;
            --scan-password-hash)
                scanpasswordhash="${argv[$i]}" ;;
            --aes-key)
                aeskey="${argv[$i]}" ;;
            --m4-root-hash)
                m4roothash="${argv[$i]}" ;;
            --m4-aes-key)
                m4aeskey="${argv[$i]}" ;;
            --enable-sb)
                enablesb="${argv[$i]}" ;;
            --enable-aes)
                enableaes="${argv[$i]}" ;;
            --enable-jtag-password)
                enablejtagpassword="${argv[$i]}" ;;
            --enable-usb-password)
                enableusbpassword="${argv[$i]}" ;;
            --enable-scan-password)
                enablescanpassword="${argv[$i]}" ;;
            --enable-anti-rollback)
                enableantirollback="${argv[$i]}" ;;
            --disable-boot-usb)
                disablebootusb="${argv[$i]}" ;;
            --disable-boot-spi)
                disablebootspi="${argv[$i]}" ;;
            --disable-boot-sdcard)
                disablebootsdcard="${argv[$i]}" ;;
            --disable-boot-nand-emmc)
                disablebootnandemmc="${argv[$i]}" ;;
            --disable-boot-recover)
                disablebootrecover="${argv[$i]}" ;;
            --disable-print)
                disableprint="${argv[$i]}" ;;
            --disable-jtag)
                disablejtag="${argv[$i]}" ;;
            --disable-scan-chain)
                disablescanchain="${argv[$i]}" ;;
            --revoke-rsk-0)
                revokersk0="${argv[$i]}" ;;
            --revoke-rsk-1)
                revokersk1="${argv[$i]}" ;;
            --revoke-rsk-2)
                revokersk2="${argv[$i]}" ;;
            --revoke-rsk-3)
                revokersk3="${argv[$i]}" ;;
            --user-efuse-file)
                userefusefile="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            --sig-ver)
                sigver="${argv[$i]}" ;;
            --key-hash-ver)
                keyhashver="${argv[$i]}" ;;
            --generate-efuse-pattern)
                i=$((i - 1))
		;;
            --raw-otp-pattern)
                opt_raw_otp_pattern="${argv[$i]}" ;;
            --soc)
                soc="${argv[$i]}" ;;
            --soc-rev)
                socrev="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done

local tool_type=gxl
local hashver=2

#check soc first, only support gxl/txlx/g12a/g12b
if [ ${soc} == "g12a" ] || [ ${soc} == "g12b" ]; then
	tool_type=g12a
	soc=g12a
else if [ ${soc} == "txlx" ] || [ ${soc} == "gxl" ] ; then
  tool_type=gxl
else
  echo invalid soc [$soc]
  exit 1
fi
fi

if [ ${soc} == "gxl" ];then
hashver=1
fi

readonly tools_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
local sign_boot_tool_dev="${tools_dir}/signing-tool-${tool_type}-dev/sign-boot-${tool_type}-dev.sh"
local efuse_gen="${tools_dir}/signing-tool-${tool_type}-dev/efuse-gen.sh"

readonly rsa_root=${amlkeypath}/root.pem
readonly rsa_root0=${amlkeypath}/root0.pem
readonly rsa_root1=${amlkeypath}/root1.pem
readonly rsa_root2=${amlkeypath}/root2.pem
readonly rsa_root3=${amlkeypath}/root3.pem
readonly kaes_bl2=${amlkeypath}/bl2aeskey

local secure_boot_option=""
local other_option=""

if [ -e "$passwordhash" ]; then
other_option="--password-hash ${passwordhash} "
fi

other_option="${other_option} --enable-usb-password $enableusbpassword --enable-jtag-password $enablejtagpassword --enable-anti-rollback $enableantirollback --raw-otp-pattern ${opt_raw_otp_pattern}"

local rsa_root_hash=${amlkeypath}/rootkeys-hash.bin

if [ -e $rsa_root0 ] ; then
"$sign_boot_tool_dev" --create-root-hash \
	--key-hash-ver $hashver                \
	--root-key-0 "$rsa_root0"              \
	--root-key-1 "$rsa_root1"              \
	--root-key-2 "$rsa_root2"              \
	--root-key-3 "$rsa_root3"              \
	-o "$rsa_root_hash"

if [ -e $rsa_root_hash ]; then
secure_boot_option="--root-hash $rsa_root_hash "
fi
fi

if [ -e ${kaes_bl2} ]; then
secure_boot_option="${secure_boot_option} --aes-key ${kaes_bl2} "
fi

secure_boot_option="${secure_boot_option} --enable-sb ${enablesb} --enable-aes ${enableaes} "

"$efuse_gen" --generate-efuse-pattern \
  --soc $soc                          \
  --key-hash-ver $hashver             \
  $secure_boot_option                 \
  $other_option                       \
  -o ${output}

rm -f $rsa_root_hash

}

parse_main() {
    case "$@" in
        --help)
            usage
            ;;
        --version)
            echo "$(basename $0) version $VERSION"
            ;;
        *-o*)
            generate_efuse_pattern "$@"
            ;;
        *)
            usage "$@"
            ;;
    esac
}

parse_main "$@"