CMakeLists.txt Template {#cmake_template}
==========

```c
# Copyright (c) 2021-2022 Amlogic, Inc. All Rights Reserved.

# SPDX-License-Identifier: MIT

if(CONFIG_XXX)							/* Compile this module conditionally, defined by Kconfig */

aml_add_library()						/* Define module name. The default module is parent_dirname__dirname. */

target_compile_options(${CURRENT_LIBRARY} PUBLIC -DXXX ...)	/* Add extra compile options if needed */

aml_library_sources(						/* Add the file list of source code to be compiled */
       ...
)

aml_library_include_directories(				/* Add the directories of header files */
       ...
)

aml_library_link_libraries(lib__xxx)				/* Add dependencies of this module */
...

aml_link_libraries()						/* Add this module to the final target */

endif()
```
