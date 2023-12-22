
source fip/variables.sh

in_bootloade=0

__mk() {
	local cur prev

	COMPREPLY=()
	cur=`_get_cword`
	prev=${COMP_WORDS[COMP_CWORD-1]}

	case "$prev" in
	./mk)
		COMPREPLY=( $( compgen -W "$(ls ${BL33_DEFCFG1}/ ${BL33_DEFCFG2}/ \
			${BL33_DEFCFG3}/ 2>/dev/null |sed -n '/_defconfig/p;'|sed 's/_defconfig//g') " -- $cur) )

		if [[ -z $COMPREPLY ]]; then
			#echo "not generating COMPREPLY or not in bootloader"
			in_bootloader=0
		else
			#echo "Generating COMPREPLY in bootloader"
			in_bootloader=1
		fi
	;;
	*)
		if  [[ $in_bootloader == 1 ]]; then
			COMPREPLY=( $( compgen -W "$(echo ${COMPILE_PARA_LIST[@]})" -- $cur) )
		fi

	;;
	esac
}

complete -F __mk mk
complete -r ./mk 2>/dev/null
