# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_SOC_A5)
aml_library_sources_ifdef(
	CONFIG_MAILBOX
	mailbox.c
	mailbox-irq.c
	mailbox-htbl.c
	rpc-user.c
)
else()
aml_library_sources_ifdef(
	CONFIG_MAILBOX
	mailbox-pl.c
	mailbox-htbl.c
	rpc-user.c
)
endif()

