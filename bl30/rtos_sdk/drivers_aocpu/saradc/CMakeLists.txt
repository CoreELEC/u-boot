# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT
if(CONFIG_SOC_A5 OR CONFIG_SOC_A4)
aml_library_sources_ifdef(
	CONFIG_SARADC
	saradc_c2.c
)
else()
aml_library_sources_ifdef(
	CONFIG_SARADC
	saradc.c
)
endif()

