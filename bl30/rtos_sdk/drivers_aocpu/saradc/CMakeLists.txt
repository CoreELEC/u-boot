# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT
if(CONFIG_SOC_A5 OR CONFIG_SOC_A4)
aml_library_sources_ifdef(
	CONFIG_SARADC
	saradc_c2.c
)
elseif(CONFIG_SOC_S7)
aml_library_sources_ifdef(
	CONFIG_SARADC
	saradc_s7.c
)
else()
aml_library_sources_ifdef(
	CONFIG_SARADC
	saradc.c
)
endif()

