aml_library_sources(
    port.c
	xlat_tables/xlat_tables_common.c
	xlat_tables/xlat_tables.c
    portASM.S
)

aml_library_include_directories(
	${CMAKE_CURRENT_LIST_DIR}
)
