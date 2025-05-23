# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

CAMKE_CACHE_FILE = $(kernel_BUILD_DIR)/CMakeCache.txt
PATH_NM = $(shell awk -F':FILEPATH=' '/CMAKE_NM_COMPILER/ {print $$2}' $(1))

quiet = silent_
nm := $(call PATH_NM,$(CAMKE_CACHE_FILE))
cmd = $(if $($(quiet)cmd_$(1)),echo '  $($(quiet)cmd_$(1))' &&) $(cmd_$(1))

SYSTEM_MAP = $(nm) $(1) | \
		grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | \
		LC_ALL=C sort | sed 's/0000000000//'

cmd_smap ?= \
	smap_addr=`$(call SYSTEM_MAP,${kernel_BUILD_DIR}/freertos.elf) | \
		awk '$$2 ~ /[tTwW]/ {printf $$1 " "}'` ; \
	smap_sym=`$(call SYSTEM_MAP,${kernel_BUILD_DIR}/freertos.elf) | \
		awk '$$2 ~ /[tTwW]/ {printf $$3 " "}'` ; \
	echo "char const system_map_addr[] = {" > $(SDK_BASE)/lib/backtrace/system_map_addr.c;\
	echo "\"$${smap_addr}\"\"\\\\000\"};" >> $(SDK_BASE)/lib/backtrace/system_map_addr.c;\
	echo "char const system_map_sym[] = {" > $(SDK_BASE)/lib/backtrace/system_map_sym.c;\
	echo "\"$${smap_sym}\"\"\\\\000\"};" >> $(SDK_BASE)/lib/backtrace/system_map_sym.c;

.PHONY: backtrace
backtrace:
	@echo "Extracted backtrace toolchain: $(nm)"
	@$(call cmd,smap)

.PHONY: clean
clean:
	@truncate -s 0 $(SDK_BASE)/lib/backtrace/system_map_addr.c
	@truncate -s 0 $(SDK_BASE)/lib/backtrace/system_map_sym.c
	@echo "char const system_map_addr[] = {0};" > $(SDK_BASE)/lib/backtrace/system_map_addr.c
	@echo "char const system_map_sym[] = {0};" > $(SDK_BASE)/lib/backtrace/system_map_sym.c
