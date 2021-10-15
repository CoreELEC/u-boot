cmake_minimum_required(VERSION 3.13.1)
project(unisoc-freertos)

include($ENV{SDK_BASE}/build/cmake/root.cmake)

target_include_directories(${TARGET_NAME} PUBLIC include)
