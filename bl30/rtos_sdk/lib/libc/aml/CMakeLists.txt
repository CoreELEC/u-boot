if(CONFIG_LIBC_AML)

aml_add_library()

aml_library_sources(
	aml_printf.c
	aml_malloc.c
	aml_free.c
	aml_memcpy.c
	aml_memset.c
	aml_strlen.c
	)

aml_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${SDK_BASE}/boards/${ARCH}/${BOARD}
	${SDK_BASE}/kernel/freertos/include

)

aml_library_link_libraries(kernel__${KERNEL})
aml_library_link_libraries(drivers__${ARCH})
aml_link_libraries()

endif()

