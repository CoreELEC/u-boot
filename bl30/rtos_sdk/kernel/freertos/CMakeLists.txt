aml_add_library()

aml_library_sources(
	list.c
	queue.c
	tasks.c
	event_groups.c
	timers.c
	portable/MemMang/heap_5.c
)

aml_library_sources(
	portable/GCC/${CONFIG_KERNEL_ARCH_DIR}/port.c
	portable/GCC/${CONFIG_KERNEL_ARCH_DIR}/portASM.S
)

if(CONFIG_ARM_CA53_64_BIT)
aml_library_sources(
	portable/GCC/${CONFIG_KERNEL_ARCH_DIR}/xlat_tables/xlat_tables_common.c
	portable/GCC/${CONFIG_KERNEL_ARCH_DIR}/xlat_tables/xlat_tables.c
)
endif()

if(CONFIG_ARM_CA53_64_BIT)
aml_library_include_directories(
	${SDK_BASE}/include/drivers
)
endif()

aml_library_include_directories(
	include
	portable/GCC/${CONFIG_KERNEL_ARCH_DIR}
	${SDK_BASE}/include
	${SDK_BASE}/products/${PRODUCT}/include
	${SDK_BASE}/soc/${ARCH}
	${SDK_BASE}/soc/${ARCH}/${CONFIG_SOC_NAME}
	${SDK_BASE}/boards/${ARCH}/${BOARD}
)

aml_link_libraries()
