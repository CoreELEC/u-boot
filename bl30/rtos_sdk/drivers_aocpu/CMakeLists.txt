set(CURRENT_LIBRARY drivers)
message(STATUS "@@CURRENT_LIBRARY: ${CURRENT_LIBRARY}")

add_library(${CURRENT_LIBRARY} STATIC)

add_subdirectory_if_kconfig(get_version)
add_subdirectory_if_kconfig(stick_mem)
add_subdirectory_if_kconfig(uart)
add_subdirectory_if_kconfig(cec)
add_subdirectory_if_kconfig(ddr)
add_subdirectory_if_kconfig(mailbox)
add_subdirectory_if_kconfig(vrtc)
add_subdirectory_if_kconfig(str)
add_subdirectory_if_kconfig(gpio)
add_subdirectory_if_kconfig(key)
add_subdirectory_if_kconfig(pwm)
add_subdirectory_if_kconfig(timer_source)
add_subdirectory_if_kconfig(saradc)
add_subdirectory_if_kconfig(ir)

aml_library_include_directories(
	${SDK_BASE}/soc/${ARCH}
	${SDK_BASE}/soc/${ARCH}/${SOC}
	${SDK_BASE}/drivers_aocpu/get_version
	${SDK_BASE}/drivers_aocpu/stick_mem
	${SDK_BASE}/drivers_aocpu/uart
	${SDK_BASE}/drivers_aocpu/cec
	${SDK_BASE}/drivers_aocpu/ddr
	${SDK_BASE}/drivers_aocpu/mailbox
	${SDK_BASE}/drivers_aocpu/vrtc
	${SDK_BASE}/drivers_aocpu/str
	${SDK_BASE}/drivers_aocpu/gpio
	${SDK_BASE}/drivers_aocpu/key
	${SDK_BASE}/drivers_aocpu/pwm
	${SDK_BASE}/drivers_aocpu/timer_source
	${SDK_BASE}/drivers_aocpu/saradc
	${SDK_BASE}/drivers_aocpu/ir
	${SDK_BASE}/drivers_aocpu/common
)

aml_library_link_libraries(arch__${ARCH})
aml_library_link_libraries(boards__${ARCH})

aml_link_libraries()

