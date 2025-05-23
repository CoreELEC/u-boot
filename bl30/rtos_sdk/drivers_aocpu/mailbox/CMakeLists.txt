# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if((CONFIG_SOC_T5D) OR (CONFIG_SOC_T5W) OR (CONFIG_SOC_TXHD2))
aml_library_sources_ifdef(
	CONFIG_MAILBOX
	mailbox-pl.c
	mailbox-htbl.c
	mailbox-rpc-test.c
)
else()
aml_library_sources_ifdef(
	CONFIG_MAILBOX
	mailbox.c
	mailbox-irq.c
	mailbox-htbl.c
	mailbox-rpc-test.c
)
endif()

