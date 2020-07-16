ARCH_DIRS="n200"

function list_boards()
{
	for arch in ${ARCH_DIRS}; do
		for board in demos/amlogic/${arch}/*; do
			if [ -d ${board} -a -e ${board}/config.mk ]; then
				echo `basename "${board}"`
			fi
		done
	done
}

function get_arch()
{
	board="$1"
	for arch in ${ARCH_DIRS}; do
		if [ -d demos/amlogic/${arch}/${board} -a -e demos/amlogic/${arch}/${board}/config.mk ]; then
			echo ${arch}
			break;
		fi
	done
}
