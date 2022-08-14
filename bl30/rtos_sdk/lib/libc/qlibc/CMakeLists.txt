# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_QLIBC)

aml_add_library()

aml_library_include_directories(
	src/internal
	src/internal/md5/
	include/qlibc/
)

aml_library_sources(
	src/containers/qgrow.c
	src/containers/qhasharr.c
	src/containers/qhashtbl.c
	src/containers/qlist.c
	src/containers/qlisttbl.c
	src/containers/qqueue.c
	src/containers/qstack.c
	src/containers/qvector.c
	src/containers/qtreetbl.c

	src/internal/qinternal.c
	src/internal/md5/md5c.c

#	src/ipc/qsem.c
#	src/ipc/qshm.c

	src/utilities/qcount.c
	src/utilities/qencode.c
	src/utilities/qfile.c
	src/utilities/qhash.c
	src/utilities/qio.c
#	src/utilities/qsocket.c
	src/utilities/qstring.c
#	src/utilities/qsystem.c
	src/utilities/qtime.c

	src/extensions/qaconf.c
	src/extensions/qconfig.c
	src/extensions/qdatabase.c
#	src/extensions/qhttpclient.c
	src/extensions/qlog.c
	src/extensions/qtokenbucket.c
)

if (CONFIG_QLIBC_TEST)
aml_library_include_directories(
	tests
)

aml_library_sources(
	tests/test_qhasharr.c
	tests/test_qhasharr_darkdh.c
	tests/test_qhashtbl.c
	tests/test_qlist.c
	tests/test_qqueue.c
	tests/test_qstack.c
	tests/test_qstring.c
	tests/test_qtreetbl.c
	tests/test_qvector.c
)
endif()

aml_library_link_libraries(lib__posix)

aml_link_libraries()

endif()