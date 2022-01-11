# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

aml_library_sources_ifdef(
	CONFIG_PWM
	pwm_meson_v2.c
	pwm_${SOC}_plat.c
)

