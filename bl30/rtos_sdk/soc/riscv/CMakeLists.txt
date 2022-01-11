# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_add_interface()

aml_interface_include_directories(
	${SDK_BASE}/soc/${ARCH}
	${SDK_BASE}/soc/${ARCH}/${SOC}
)

aml_link_libraries()
