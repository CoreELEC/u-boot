cmake_minimum_required(VERSION 3.13.1)
project(aml-rtos)

include($ENV{SDK_BASE}/build/cmake/root.cmake)

target_include_directories(${TARGET_NAME} PUBLIC include)

aml_sources(
	util.c
	fsm.c
	btwake.c
	keypad.c
	power.c
	main.c
)

