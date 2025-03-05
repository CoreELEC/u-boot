# Copyright (c) 2025 Hardkernel Co., Ltd. All rights reserved.

# SPDX-License-Identifier: MIT

aml_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

aml_library_sources(
	power.c
	hw_business.c
)

aml_library_sources_ifdef(
	CONFIG_LEDS
	leds_plat.c
)
