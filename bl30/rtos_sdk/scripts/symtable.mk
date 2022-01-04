B_PRJDIR := $(SDK_BASE)
kernel_B_BUILD_DIR := $(kernel_BUILD_DIR)

NM:=$(TOOLCHAIN_KEYWORD)-nm
CC:=$(TOOLCHAIN_KEYWORD)-$(COMPILER)

quiet=silent_
cmd = $(if $($(quiet)cmd_$(1)),echo '  $($(quiet)cmd_$(1))' &&) $(cmd_$(1))

SYSTEM_MAP = $(NM) $(1) | \
		grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | \
		LC_ALL=C sort | sed 's/0000000000//'

cmd_smap ?= \
	smap_addr=`$(call SYSTEM_MAP,${kernel_B_BUILD_DIR}/freertos.elf) | \
		awk '$$2 ~ /[tTwW]/ {printf $$1 " "}'` ; \
	smap_sym=`$(call SYSTEM_MAP,${kernel_B_BUILD_DIR}/freertos.elf) | \
		awk '$$2 ~ /[tTwW]/ {printf $$3 " "}'` ; \
	echo "char const system_map_addr[] = {" > $(B_PRJDIR)/libs/stack_trace/system_map_addr.c;\
	echo "\"$${smap_addr}\"\"\\\\000\"};" >> $(B_PRJDIR)/libs/stack_trace/system_map_addr.c;\
	echo "char const system_map_sym[] = {" > $(B_PRJDIR)/libs/stack_trace/system_map_sym.c;\
	echo "\"$${smap_sym}\"\"\\\\000\"};" >> $(B_PRJDIR)/libs/stack_trace/system_map_sym.c;

.PHONY: backtrace
backtrace:
	@$(call cmd,smap)