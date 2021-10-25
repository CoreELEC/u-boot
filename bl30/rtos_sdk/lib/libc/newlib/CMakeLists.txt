if(CONFIG_LIBC_STD)

aml_add_library()

aml_library_sources(
	syscalls.c
)

aml_library_link_libraries(kernel__${KERNEL})
aml_library_link_libraries(drivers__${ARCH})
target_link_libraries(${TARGET_NAME} -lc)
target_link_libraries(${TARGET_NAME} libc__newlib)
target_link_libraries(${TARGET_NAME} -lc)

endif()

