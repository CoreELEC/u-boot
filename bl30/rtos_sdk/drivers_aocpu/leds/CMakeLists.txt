# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_library_sources_ifdef(
	CONFIG_LEDS
	leds_state.c
	leds_${SOC}_plat.c
)

