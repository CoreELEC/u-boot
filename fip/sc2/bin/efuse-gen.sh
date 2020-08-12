 #!/bin/bash

EXEC_BASEDIR=$(dirname $(readlink -f $0))
BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

#
# Settings
#
VERSION=0.2

# Check file
check_file() {
    if [ ! -f "$2" ]; then echo Error: Unable to open $1: \""$2"\"; exit 1 ; fi
}

# Check file is size or exit. $1: file, $2: size
check_size() {
    local filesize=$(wc -c < "$1")
    if [ $filesize -ne $2 ]; then
        echo "Error: File \"$1\" incorrect size. Was $filesize, expected $2"
        exit 1
    fi
}

# Check optional file argument exists and is given size
# $1 arg name
# $2 size
# $3 file
check_opt_file() {
    if [ -n "$3" ]; then
        check_file "$1" "$3"
        local filesize=$(wc -c < "$3")
        if [ $filesize -ne $2 ]; then
            echo "Incorrect size $filesize != $2 for $1 $3"
            exit 1
        fi
    fi
}

usage() {
    cat << EOF
Usage: $(basename $0) --help
       $(basename $0) --version
       $(basename $0) --input base.efuse.bin \\
                      [--device-roothash device_roothash.bin] \\
                      [--dvgk dvgk.bin] \\
                      -o pattern.efuse
       $(basename $0) --audio-id audio_id_value \\
                      -o audio_id.efuse
EOF
    exit 1
}

function generate_efuse_device_pattern() {
    local argv=("$@")
    local i=0

    local patt=$(mktemp --tmpdir)
    local wrlock0=$(mktemp --tmpdir)
    local wrlock1=$(mktemp --tmpdir)
	local hmac=$(mktemp --tmpdir)

    # Parse args
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
				#echo "i=$i argv[$i]=${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --input)
                input="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
			--dvgk)
                dvgk="${argv[$i]}" ;;
			--device-roothash)
                device_roothash="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done

    # Verify args
    if [ -z "$output" ]; then echo Error: Missing output file option -o; exit 1; fi

	check_file input "$input"
	check_size "$input" 4096

	check_opt_file dvgk 16 "$dvgk"
	check_opt_file device_roothash 32 "$device_roothash"

    # Generate empty eFUSE pattern data
    dd if="$input" of=$patt count=4096 bs=1 &> /dev/null

    # Construct wrlock bits
    b_1e2="00"
    b_1e3="00"
    b_1fc="00"

    if [ "$dvgk" != "" ]; then
		${EXEC_BASEDIR}/vendor-keytool gen-mrk-chknum --chipset=SC2 --mrk-file="$dvgk" --mrk-name=DVGK | grep 'Long checknum: ' | \
            grep "Long checknum:" | sed 's/Long checknum: //' | sed 's/ (.*//' | xxd -r -p > $hmac

		dd if="$dvgk" of="$patt" bs=16 seek=226 count=1 \
            conv=notrunc >& /dev/null
		dd if="$hmac" of="$patt" bs=16 seek=194 count=1 \
            conv=notrunc >& /dev/null
		b_1fc="$(printf %02x $(( 0x$b_1fc | 0x04 )))"
    fi

    if [ "$device_roothash" != "" ]; then
        dd if="$device_roothash" of="$patt" bs=16 seek=23 count=2 \
            conv=notrunc >& /dev/null
	    dd if="$device_roothash" of="$patt" bs=16 seek=25 count=2 \
            conv=notrunc >& /dev/null
		b_1e2="$(printf %02x $(( 0x$b_1e2 | 0x80 )))"
		b_1e3="$(printf %02x $(( 0x$b_1e3 | 0x07 )))"
    fi

    echo 00 00 $b_1e2 $b_1e3 00 00 00 00 00 00 00 00 00 00 00 00 | xxd -r -p > $wrlock0
    echo 00 00 00 00 00 00 00 00 00 00 00 00 $b_1fc 00 00 00 | xxd -r -p > $wrlock1

    filesize=$(wc -c < $wrlock0)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid write-lock0 pattern length
        exit 1
    fi
    dd if=$wrlock0 of=$patt bs=16 seek=30 count=1 conv=notrunc >& /dev/null

    filesize=$(wc -c < $wrlock1)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid write-lock1 pattern length
        exit 1
    fi
    dd if=$wrlock1 of=$patt bs=16 seek=31 count=1 conv=notrunc >& /dev/null

	${BASEDIR_TOP}/aml_encrypt_sc2 --efsproc --input $patt --output $output --option=debug

    rm -f $patt
    rm -f $wrlock0
	rm -f $wrlock1
}

function append_uint32_le() {
    local input=$1
    local output=$2
    local v=
    local vrev=
    v=$(printf %08x $input)
    # 00010001
    vrev=${v:6:2}${v:4:2}${v:2:2}${v:0:2}

    echo $vrev | xxd -r -p >> $output
}

function generate_audio_id_pattern() {
    local argv=("$@")
    local i=0
    local patt=$(mktemp --tmpdir)
    local audio_id_efuse=$(mktemp --tmpdir)
    # default audio_id_offset 0xB8
    local audio_id_offset=184
    local audio_id_size=4
     # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        #echo "i=$i argv[$i]=${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --audio-id)
                audio_id_value="${argv[$i]}" ;;
           -o)
                output="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done

    # Verify args
    if [ -z "$output" ]; then echo Error: Missing output file option -o; exit 1; fi

    if [ -z $audio_id_value ]; then
        echo Error: invalid audio_id_value
        exit 1
    fi

    # Generate empty eFUSE pattern data
    dd if=/dev/zero of=$patt count=4096 bs=1 &> /dev/null

    append_uint32_le $audio_id_value $audio_id_efuse
    dd if=$audio_id_efuse of=$patt bs=1 seek=$audio_id_offset count=$audio_id_size \
        conv=notrunc >& /dev/null

	${BASEDIR_TOP}/aml_encrypt_sc2 --efsproc --input $patt --output $output --option=debug

    rm -f $patt
    rm -f $audio_id_efuse
}

parse_main() {
    case "$@" in
        --help)
            usage
            ;;
        --version)
            echo "$(basename $0) version $VERSION"
            ;;
        *--audio-id*)
            generate_audio_id_pattern "$@"
            ;;
        *-o*)
            generate_efuse_device_pattern "$@"
            ;;
        *)
            usage "$@"
            ;;
    esac
}

parse_main "$@"
