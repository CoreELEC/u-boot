aml_add_library()

aml_library_sources(
	${BOARD}/fsm.c
	${BOARD}/btwake.c
	${BOARD}/keypad.c
	${BOARD}/power.c
	${BOARD}/hw_business.c
)

aml_library_link_libraries(kernel__${KERNEL})
aml_library_link_libraries(drivers__${ARCH})

aml_link_libraries()
