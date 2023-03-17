# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

set(CURRENT_LIBRARY_TYPE STATIC)
set(CURRENT_LIBRARY drivers)
message(STATUS "@@${CURRENT_LIBRARY_TYPE}: ${CURRENT_LIBRARY}")

aml_add_library_spec(${CURRENT_LIBRARY} ${CURRENT_LIBRARY_TYPE})

add_subdirectory_if_kconfig(stick_mem)
add_subdirectory_if_kconfig(uart)
add_subdirectory_if_kconfig(cec)
add_subdirectory_if_kconfig(ddr)
add_subdirectory_if_kconfig(mailbox)
add_subdirectory_if_kconfig(rtc)
add_subdirectory_if_kconfig(vrtc)
add_subdirectory_if_kconfig(str)
add_subdirectory_if_kconfig(gpio)
add_subdirectory_if_kconfig(key)
add_subdirectory_if_kconfig(pwm)
add_subdirectory_if_kconfig(leds)
add_subdirectory_if_kconfig(timer_source)
add_subdirectory_if_kconfig(saradc)
add_subdirectory_if_kconfig(ir)
add_subdirectory_if_kconfig(hwspinlock)
add_subdirectory_if_kconfig(i2c)
add_subdirectory_if_kconfig(ethernet)
add_subdirectory_if_kconfig(hdmirx)

aml_library_include_directories(
	${SDK_BASE}/drivers_aocpu/stick_mem
	${SDK_BASE}/drivers_aocpu/uart
	${SDK_BASE}/drivers_aocpu/cec
	${SDK_BASE}/drivers_aocpu/ddr
	${SDK_BASE}/drivers_aocpu/mailbox
	${SDK_BASE}/drivers_aocpu/rtc
	${SDK_BASE}/drivers_aocpu/vrtc
	${SDK_BASE}/drivers_aocpu/str
	${SDK_BASE}/drivers_aocpu/gpio
	${SDK_BASE}/drivers_aocpu/key
	${SDK_BASE}/drivers_aocpu/pwm
	${SDK_BASE}/drivers_aocpu/leds
	${SDK_BASE}/drivers_aocpu/timer_source
	${SDK_BASE}/drivers_aocpu/saradc
	${SDK_BASE}/drivers_aocpu/ir
	${SDK_BASE}/drivers_aocpu/hwspinlock
	${SDK_BASE}/drivers_aocpu/i2c
	${SDK_BASE}/drivers_aocpu/ethernet
	${SDK_BASE}/drivers_aocpu/hdmirx
)

aml_library_link_libraries(arch__${ARCH})
aml_library_link_libraries(soc__${ARCH})
aml_library_link_libraries(boards__${ARCH})

aml_link_libraries()

