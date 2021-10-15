aml_library_sources(
	start.S
	vtable.S
	portasm.S
	port.c
)

aml_library_include_directories(
	${CMAKE_CURRENT_LIST_DIR}
)
