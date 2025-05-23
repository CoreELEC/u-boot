How to Add a New Library {#add_library}
==========

Let's take ***ULog*** library for example.

### Step 1: Prepare your source code. ###
Make directory ***libs/ulog***, and put your source code into it.

	mkdir -p libs/ulog

### Step 2: Add ***Kconfig***. ###
Write ***Kconfig*** for your library.

@code
config ULOG
    bool "Ulog"
    default n
    select COMMON_IO
    select OSAL
    select POSIX
        help
          The log is to output the status, process and other information of the software to different media, display and save.
@endcode

Then add the following line to root ***Kconfig***.
@code
source "libs/ulog/Kconfig"
@endcode

### Step 3: Add ***CMakeLists.txt***. ###
Write ***CMakeLists.txt*** for your library.

@code
if(CONFIG_ULOG)

aml_add_library()

set(src_dir "${CMAKE_CURRENT_LIST_DIR}")
set(inc_dir "${CMAKE_CURRENT_LIST_DIR}/include")

aml_library_sources(
        "${src_dir}/ulog.c"
        "${src_dir}/ringblk_buf.c"
        "${src_dir}/single_list.c"
        "${src_dir}/backend/console_be.c"
        "${src_dir}/syslog/syslog.c"
)

aml_library_include_directories(
        "${inc_dir}"
)

aml_library_link_libraries(lib__posix)
aml_library_link_libraries(lib__osal)
aml_library_link_libraries(lib__common_io)
aml_link_libraries()

endif()
@endcode

Then add the following line to root ***CMakeLists.txt***.
@code
add_subdirectory(libs/ulog)
@endcode

### Step 4: Enable the new libs by default. ###
Let's take ***products/aocpu*** for example.

Add the following line to ***products/aocpu/prj.conf***.

@code
CONFIG_ULOG=y
@endcode
