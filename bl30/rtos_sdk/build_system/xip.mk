#Object files and dependency definitions
B_PRJDIR := $(RTOS_BUILD_DIR)
kernel_B_BUILD_DIR := $(B_PRJDIR)/output/$(ARCH)-$(BOARD)-$(PRODUCT)/$(KERNEL)
RTOS_ELF = $(kernel_B_BUILD_DIR)/${KERNEL}.elf
RTOSDemo_a = $(kernel_B_BUILD_DIR)/${KERNEL}_a.bin
RTOSDemo_b = $(kernel_B_BUILD_DIR)/${KERNEL}_b.bin
RTOSDemo = $(kernel_B_BUILD_DIR)/${KERNEL}.bin

#toolchain
NM:=$(TOOLCHAIN_KEYWORD)-nm
OBJCOPY:=$(TOOLCHAIN_KEYWORD)-objcopy
CC:=$(TOOLCHAIN_KEYWORD)-$(COMPILER)

#Specify link segment
_membdatalist+=.mem_text .data .bss .stack .heap

#Generate the specified bin file
RTOSDemo_bin=${OBJCOPY} -O binary $(addprefix -R ,$(_membdatalist)) ${RTOS_ELF} ${RTOSDemo_b};
RTOSDemo_bin+=${OBJCOPY} -O binary $(addprefix -j ,$(_membdatalist)) ${RTOS_ELF} ${RTOSDemo_a};
RTOSDemo_bin+=cat ${RTOSDemo_a} > ${RTOSDemo};
RTOSDemo_bin+=printf '%08X' $$(stat -c "%s" ${RTOSDemo})|tac -brs ..|cut -b-8|xxd -r -p|dd of=${RTOSDemo} bs=1 seek=4 count=4 conv=notrunc;


.PHONY: xip
xip:
	@$(RTOSDemo_bin)
