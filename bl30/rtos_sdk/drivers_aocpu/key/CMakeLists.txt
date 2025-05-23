# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_library_sources_ifdef(
	CONFIG_KEY
	gpio_key.c
	dynamic_keypad.c
)

aml_library_sources_ifndef(
	CONFIG_BOARD_S7D_ODROIDC5
	adc_key.c
)
