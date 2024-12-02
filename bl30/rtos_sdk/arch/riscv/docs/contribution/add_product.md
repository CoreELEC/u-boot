How to Add a New Product	{#add_product}
==========

Let's take ***aocpu*** for example.

### Step 1: Prepare your source code. ###
Make directory ***products/aocpu***, and put your **main.c** into it.

	mkdir -p products/aocpu

### Step 2: Add ***CMakeLists.txt***. ###
Write ***CMakeLists.txt*** and put it into ***products/aocpu***.

@code
cmake_minimum_required(VERSION 3.13.1)
project(aml-rtos)

include($ENV{SDK_BASE}/build_system/cmake/root.cmake)

target_include_directories(${TARGET_NAME} PUBLIC include)

aml_sources(
	main.c
	...
)
@endcode

Please change the include directories, source code and dependent libraries accordingly.

### Step 3: Add ***prj.conf***. ###
Write ***prj.conf*** and put it into ***products/aocpu***.

@code
#CONFIG_LIBC_AML and CONFIG_LIBC_STD must has one to be selected
CONFIG_LIBC=y
...
@endcode

Note that ***prj.conf*** <span style="color:red">defines the default **software** features</span>.\n
Please change the features accordingly.
