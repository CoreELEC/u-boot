# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

aml_library_sources(
	fsm.c
	btwake.c
	keypad.c
	power.c
	hw_business.c
)

aml_library_sources_ifdef(
	CONFIG_LEDS
	leds_plat.c
)