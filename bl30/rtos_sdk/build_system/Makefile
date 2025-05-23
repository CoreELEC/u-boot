# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

################################################################
# Root Makefile of the whole project
################################################################

################################################################
# Basic Definitions
################################################################
BOOT		:=
KERNEL		?= freertos
ifeq ($(DOCS_ARCH),)
	ifeq ($(ARCH),)
		DOCS_ARCH = arm64
	else
		DOCS_ARCH = $(ARCH)
	endif
endif

################################################################
# Directories and Files
################################################################
SHELL		:= /bin/bash
PWD		:= $(shell pwd)
PRJDIR		:= $(PWD)

app_DIR		:= $(PRJDIR)/apps
bootloader_DIR	:= $(PRJDIR)/$(BOOT)
build_DIR	:= $(PRJDIR)/build_system
docs_DIR	:= $(PRJDIR)/arch/$(DOCS_ARCH)/docs
kernel_DIR	:= $(PRJDIR)/kernel/$(KERNEL)
fw_DIR		:= $(PRJDIR)/firmware
product_DIR	:= $(PRJDIR)/products
sign_tool_DIR	:= $(PRJDIR)/tools/sign_tool
adnl_DIR	:= $(PRJDIR)/tools/adnl

OUTPUT_DIR		:= $(PRJDIR)/output
BUILD_DIR		:= $(OUTPUT_DIR)/$(ARCH)-$(BOARD)-$(PRODUCT)
docs_BUILD_DIR		:= $(OUTPUT_DIR)/docs/$(DOCS_ARCH)
bootloader_BUILD_DIR	:= $(BUILD_DIR)/$(BOOT)
kernel_BUILD_DIR	:= $(BUILD_DIR)/$(KERNEL)
sign_tool_BUILD_DIR	:= $(sign_tool_DIR)
adnl_BUILD_DIR		:= $(adnl_DIR)

bootloader_BIN		:= $(bootloader_BUILD_DIR)/$(BOOT)/ext/mcuboot/mcuboot.bin
kernel_BIN		:= $(kernel_BUILD_DIR)/$(KERNEL).bin
fw_BIN			:= $(fw_DIR)/wcn-modem.bin
adnl_BIN		:= $(adnl_BUILD_DIR)/adnl

DIST_DIR		:= $(BUILD_DIR)/images
bootloader_dist_BIN	:= $(DIST_DIR)/mcuboot-signed.bin
kernel_dist_BIN		:= $(DIST_DIR)/$(KERNEL)-signed.bin
fw_dist_BIN		:= $(DIST_DIR)/wcn-modem.bin
adnl_dist_BIN		:= $(DIST_DIR)/adnl

bootloader_KEYPATH	= $(build_DIR)/key/rsa/3072
kernel_KEYPATH		= $(build_DIR)/key/rsa/2048
SIGNTOOL		= $(sign_tool_DIR)/imgtool.py

SDK_BASE	:= $(PRJDIR)
TOOLCHAIN_DIR	:= $(OUTPUT_DIR)/toolchains/$(COMPILER)-$(TOOLCHAIN_KEYWORD)
PATH		:= $(TOOLCHAIN_DIR)/bin:$(PATH)

#This code segment is used to set the path of the compiler for the find_compiler.cmake to search.
ifeq ($(ARCH),xtensa)
	XTENSA_LOWPOWER_PATTERN=_lowpower
	ifeq ($(SOC),$(filter $(SOC),a5 t3x s6))
		XTENSA_CORE = Amlogic_v9
		XTENSA_TOOLCHAIN_VER = RI-2020.5-linux
	else ifeq ($(SOC),a1)
	ifeq ($(findstring $(XTENSA_LOWPOWER_PATTERN),$(BOARD)),$(XTENSA_LOWPOWER_PATTERN))
		XTENSA_CORE = Amlogic_v2
	else
		XTENSA_CORE = Amlogic_v0
	endif
		XTENSA_TOOLCHAIN_VER = RG-2018.9-linux
	else
		XTENSA_CORE = Amlogic_v0
		XTENSA_TOOLCHAIN_VER = RG-2018.9-linux
	endif
	TOOLCHAIN_PATH=/opt/xtensa/XtDevTools/install/tools/$(XTENSA_TOOLCHAIN_VER)/XtensaTools
	XTENSA_SYSTEM=/opt/xtensa/XtDevTools/install/builds/$(XTENSA_TOOLCHAIN_VER)/$(XTENSA_CORE)/config
endif

export PATH ARCH SOC BOARD KERNEL SDK_BASE OUTPUT_DIR kernel_BUILD_DIR TOOLCHAIN_PATH XTENSA_CORE XTENSA_SYSTEM

################################################################
# Macros
################################################################
# MESSAGE Macro -- display a message in bold type
MESSAGE = echo "$(TERM_BOLD)>>> $(1)$(TERM_RESET)"
TERM_BOLD := $(shell tput smso 2>/dev/null)
TERM_RESET := ${shell tput rmso 2>/dev/null}

# Macro of Generating Buildsystem
# $(1): Target
define GENERATE_BUILDSYSTEM
$($(1)_BUILD_DIR): project
	@ mkdir -p $($(1)_BUILD_DIR)
# Auto-generate root CMakeLists.txt and Kconfig
	@ ./scripts/setup.sh
	@ if [ ! -f $($(1)_BUILD_DIR)/build.ninja ]; then \
		cmake -G Ninja -DBOARD=$(BOARD) -DCMAKE_TOOLCHAIN_FILE=$(build_DIR)/cmake/toolchains/$(ARCH)_compiler.cmake -S $(product_DIR)/$(PRODUCT) -B $($(1)_BUILD_DIR); \
	fi
endef

# Macro of Building CMake Targets
# $(1): Target
define GENERATE_CMAKE_TARGET
.PHONY: $(1)
$(1): toolchain $($(1)_BUILD_DIR)
	@ $(call MESSAGE,"Building $(1)")
	@ (if [ $(1) == kernel ]; then \
		cmake --build $($(1)_BUILD_DIR); \
	else \
		if [ $(1) == bootloader ]; then \
			if [ ! -d $(kernel_KEYPATH) ]; then \
				mkdir -p $(kernel_KEYPATH); \
			fi; \
			if [ ! -f $(kernel_KEYPATH)/*private.pem -o ! -f $(kernel_KEYPATH)/*public.pem ]; then \
				$(call MESSAGE,"Generating kernel key pair ..."); \
				python3 $(SIGNTOOL) keygen -k $(kernel_KEYPATH) -t rsa-2048; \
			fi; \
			$(call MESSAGE,"Attach kernel public key"); \
			python3 $(SIGNTOOL) getpub -k $(kernel_KEYPATH)/rsa2048-private.pem > $($(1)_DIR)/bl2/ext/mcuboot/rsa_pub_key.h; \
			if [ ! -d $($(1)_KEYPATH) ]; then \
				$(call MESSAGE,"Generating $(1) key pair ..."); \
				mkdir -p $($(1)_KEYPATH); \
			fi; \
			if [ ! -f $($(1)_KEYPATH)/*private.pem -o ! -f $($(1)_KEYPATH)/*public.pem ]; then \
				python3 $(SIGNTOOL) keygen -k $($(1)_KEYPATH) -t rsa-3072; \
			fi; \
		fi; \
		cmake $($(1)_DIR)/ -G"Unix Makefiles" -DBOARD=$(BOARD) -DPRODUCT=$(PRODUCT) -DCOMPILER=$(ARCH)-gcc; \
		cmake --build ./ -- install; \
	fi \
	)
endef

# Macro of Building Targets
# $(1): Target
define GENERATE_MAKE_TARGET
.PHONY: $(1)
$(1): toolchain
	@ $(call MESSAGE,"Building $(1)")
	@ (cd $($(1)_DIR) && $(MAKE))
endef

# Macro of Menuconfig Targets
# $(1): Target
# $(2): Target suffix
define GENERATE_MENUCONFIG_TARGET
.PHONY: $(if $(2),$(1)-$(2),$(1))
$(if $(2),$(1)-$(2),$(1)): $($(1)_BUILD_DIR)
	@ cmake --build $($(1)_BUILD_DIR) --target $(2)
endef

# Macro of Building Dist Targets
# $(1): Target
# $(2): Target suffix
define GENERATE_DIST_TARGET
.PHONY: $(if $(2),$(1)-$(2),$(1))
$(if $(2),$(1)-$(2),$(1)): $(1) $(DIST_DIR)
	@ if [ -f $($(1)_BIN) ]; then install -p $($(1)_BIN) $($(1)_$(2)_BIN); fi
endef

# Macro of Cleaning Targets
# $(1): Target
# $(2): Target suffix
define GENERATE_CLEAN_TARGET
.PHONY: $(if $(2),$(1)-$(2),$(1))
$(if $(2),$(1)-$(2),$(1)):
	@ $(call MESSAGE,"Cleaning $(1)")
	@ if [ -d $($(1)_BUILD_DIR) ]; then cmake --build $($(1)_BUILD_DIR) --target $(2); fi
endef

# Macro of Signing Image
# $(1): target binary
# $(2): header length
define SIGN_IMAGE
	if [ $(1) == bootloader ]; then \
		python3 $(SIGNTOOL) sign_bl2 -P $($(1)_KEYPATH) -H $(2) $($(1)_BIN) $($(1)_dist_BIN); \
	else \
		python3 $(SIGNTOOL) sign -k $($(1)_KEYPATH)/rsa2048-private.pem -H $(2) $($(1)_BIN) $($(1)_dist_BIN); \
	fi
endef

# Macro of Segment Image
# $(1): segment makefile
define SEGMENT_IMAGE
	@ if [ -f $(1) ]; then \
		make -f $(1) scatter; \
	fi
endef
################################################################
# Targets
################################################################
BUILDSYSTEM_TARGETS	:= kernel
CMAKE_TARGETS		:= kernel
MAKE_TARGETS		:=
MENUCONFIG_TARGETS	:= kernel
INSTALL_TARGETS		:= $(CMAKE_TARGETS)
ALL_TARGETS		:= $(CMAKE_TARGETS) $(MAKE_TARGETS)
DIST_TARGETS		:= $(addsuffix -dist,$(INSTALL_TARGETS))
CLEAN_TARGETS		:= $(addsuffix -clean,$(ALL_TARGETS))

###############################################################################################################################
# $(build_DIR)/scatter_load.mk is not present by default. when segmentation functionality is required,
# the CMakeLists within the module needing segmentation will invoke gen_scatter_lib.sh to generate $(build_DIR)/scatter_load.mk,
# which includes segmentation rules, thereby triggering this functionality.
###############################################################################################################################
.PHONY: dist
dist: all $(DIST_TARGETS)
	$(call SEGMENT_IMAGE,$(build_DIR)/scatter_load.mk)
#	$(call SIGN_IMAGE,bootloader,512)
#	$(call SIGN_IMAGE,kernel,1024)

$(DIST_DIR):
	@ install -d $(DIST_DIR)

.PHONY: all
all: $(ALL_TARGETS)

.PHONY: clean
clean: $(CLEAN_TARGETS)

ifndef_any_of = $(filter undefined,$(foreach v,$(1),$(origin $(v))))

.PHONY: distclean
distclean:
ifeq ($(call ifndef_any_of,ARCH SOC BOARD PRODUCT),)
	@ if [ -d $(BUILD_DIR) ]; then rm -rf $(BUILD_DIR); fi
else
	@ if [ -d $(OUTPUT_DIR) ]; then rm -rf $(OUTPUT_DIR); fi
endif
	@ if [ -f $(build_DIR)/scatter_load.mk ]; then rm -f $(build_DIR)/scatter_load.mk; fi

.PHONY: project
project:
ifeq ($(call ifndef_any_of,ARCH SOC BOARD PRODUCT),)
	@ $(call MESSAGE,"Building $(ARCH) $(SOC) $(BOARD) $(PRODUCT)")
else
	$(error Please execute source scripts/env.sh)
endif

################################################################
## Target command:make [docs|docs-clean] DOCS_ARCH=[arm|arm64|riscv|xtensa]
################################################################
.PHONY: docs
docs:
	@ if [ ! -d $($@_BUILD_DIR) ]; then mkdir -p $($@_BUILD_DIR); fi
	@ if [ ! -d $($@_DIR) ]; then \
		echo "$($@_DIR) is not exist,please set project first with: source scripts/env.sh"; \
	else \
		echo "Generated docs in $($@_BUILD_DIR) from  $($@_DIR)."; \
		(rm -rf $($@_BUILD_DIR)/CMakeCache.txt \
		&& cd $($@_BUILD_DIR) \
		&& cmake $($@_DIR) -DARCH_NAME=$(DOCS_ARCH) \
		-DCMAKE_TOOLCHAIN_FILE=$(build_DIR)/cmake/toolchains/$(DOCS_ARCH)_compiler.cmake \
		&& make); \
	fi

.PHONY: docs-clean
docs-clean:
ifeq ($(call ifndef_any_of,ARCH SOC BOARD PRODUCT),)
	@ if [ -d $(docs_BUILD_DIR) ]; then rm -rf $(docs_BUILD_DIR); fi
else
	@ if [ -d $(OUTPUT_DIR)/docs ]; then rm -rf $(OUTPUT_DIR)/docs; fi
endif

################################################################
# Respective Targets
################################################################
# Buildsystem Targets
$(foreach target,$(BUILDSYSTEM_TARGETS),$(eval $(call GENERATE_BUILDSYSTEM,$(target))))

# CMake Build Targets
$(foreach target,$(CMAKE_TARGETS),$(eval $(call GENERATE_CMAKE_TARGET,$(target))))

# Build Targets
$(foreach target,$(MAKE_TARGETS),$(eval $(call GENERATE_MAKE_TARGET,$(target))))

# Menuconfig Targets
#$(foreach target,$(MENUCONFIG_TARGETS),$(eval $(call GENERATE_MENUCONFIG_TARGET,$(target),menuconfig)))

# Dist Targets
$(foreach target,$(ALL_TARGETS) fw,$(eval $(call GENERATE_DIST_TARGET,$(target),dist)))

# Clean Targets
$(foreach target,$(ALL_TARGETS),$(eval $(call GENERATE_CLEAN_TARGET,$(target),clean)))

.PHONY: toolchain
toolchain:
	@ ./scripts/prepare_toolchain.sh

.PHONY: menuconfig
menuconfig: toolchain $(kernel_BUILD_DIR)
	@ cmake --build $(kernel_BUILD_DIR) --target $@

%defconfig:config-clean toolchain
	@ make $(kernel_BUILD_DIR) BOARD_SPEC_CONFIG=$@
	@ echo "configuration $@ written to $(kernel_BUILD_DIR)/.config"

.PHONY: config
config: config-clean toolchain
	@ make $(kernel_BUILD_DIR) BOARD_SPEC_CONFIG=defconfig
	@ echo "configuration defconfig written to $(kernel_BUILD_DIR)/.config"

.PHONY: config-clean
config-clean:
	@ if [ -f $(kernel_BUILD_DIR)/build.ninja ]; then \
		rm -f $(kernel_BUILD_DIR)/build.ninja; \
	fi

.PHONY: flash
flash:
	@ if [ -d $(DIST_DIR) ]; then \
		(cd $(DIST_DIR) && \
		./update_fw.sh;) \
	fi

.PHONY: backtrace
backtrace:
	@ make && \
	make -f $(build_DIR)/symtable.mk backtrace && \
	make && \
	make -f $(build_DIR)/symtable.mk clean

.PHONY: release
release:
	@ ./scripts/gen_release.sh
