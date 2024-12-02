How to Add a New Architecture	{#add_arch}
==========

Let's take ***riscv*** for example.

### Step 1: Prepare your source code. ###
Make directory ***arch/riscv***, and put your source code into it.

	mkdir -p arch/riscv

### Step 2: Add ***Kconfig***. ###
Write ***Kconfig*** and put it into ***arch/riscv***.

@code
config RISCV
	bool
	select KERNEL
	help
	  RISC-V architecture
...
@endcode

Please change the content accordingly.

Then add the following line to root ***Kconfig***.
@code
source "arch/${ARCH}/Kconfig"
@endcode

### Step 3: Add ***CMakeLists.txt***. ###
Write ***CMakeLists.txt*** and put it into ***arch/riscv***.

@code
aml_add_library()

aml_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

aml_library_sources(
	...
)

aml_library_link_libraries(kernel__${KERNEL})
...

aml_link_libraries()
@endcode
Please change the include directories, source code and dependent libraries accordingly.

Then add the following line to root ***CMakeLists.txt***.
@code
add_subdirectory(arch/${ARCH})
@endcode

### Step 4: Add ***compiler.cmake***. ###
Write ***compiler_options.cmake*** and put it into ***arch/riscv***.

@code
set(common_flags "")
set(c_flags "")

set(linker_flags "-Wl,--print-memory-usage,-Map=${TARGET_NAME}.map,--gc-sections")

if(CONFIG_LTO_OPTIMIZATION)
set(LTO_OPTIONS "-flto -ffat-lto-objects")
endif()

if(CONFIG_LIBC_STD)
set(linker_flags "${linker_flags},--wrap=_malloc_r,--wrap=_free_r,--wrap=_realloc_r,--wrap=_calloc_r")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -ffunction-sections -fdata-sections -fno-common -fgnu89-inline -march=rv32imc -mabi=ilp32 ${LTO_OPTIONS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --specs=nano.specs --specs=nosys.specs")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -ffunction-sections -fdata-sections -fno-common -fgnu89-inline -march=rv32imc -mabi=ilp32 ${LTO_OPTIONS} -nostdlib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --specs=nano.specs --specs=nosys.specs")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -O2 -g -ffunction-sections -fdata-sections -fno-common -fgnu89-inline -march=rv32imc -mabi=ilp32 ${LTO_OPTIONS} -D__ASM")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --specs=nano.specs --specs=nosys.specs")

@endcode
Please change the compiler options accordingly.

### Step 5: Add ***cross toolchain***. ###
Make directory ***arch/riscv/toolchain***, and put your cross toolchain into it.

	mkdir -p arch/riscv/toolchain

Note that the cross toolchain must be a **tar.xz** file.

Write ***riscv_compiler.cmake***, and put it into ***build_system/cmake/toolchains***.

@code
include("${CMAKE_CURRENT_LIST_DIR}/find_compiler.cmake")

set(CMAKE_SYSTEM_NAME Generic)

# Find GCC for RISCV.
aml_find_compiler(COMPILER_CC riscv-none-embed-gcc)
aml_find_compiler(COMPILER_CXX riscv-none-embed-g++)
set(COMPILER_ASM "${COMPILER_CC}" CACHE INTERNAL "")
aml_find_compiler(COMPILER_OBJCOPY riscv-none-embed-objcopy)
aml_find_compiler(COMPILER_OBJDUMP riscv-none-embed-objdump)

# Specify the cross compiler.
set(CMAKE_C_COMPILER ${COMPILER_CC} CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER ${COMPILER_CXX} CACHE FILEPATH "C++ compiler")
set(CMAKE_ASM_COMPILER ${COMPILER_ASM} CACHE FILEPATH "ASM compiler")
#set(CMAKE_C_LINK_EXECUTABLE ${COMPILER_LD} CACHE FILEPATH "LD linker")
set(CMAKE_OBJCOPY_COMPILER ${COMPILER_OBJCOPY} CACHE FILEPATH "objcopy compiler")
set(CMAKE_OBJDUMP_COMPILER ${COMPILER_OBJDUMP} CACHE FILEPATH "objdump compiler")
# Disable compiler checks.
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Add target system root to cmake find path.
get_filename_component(COMPILER_DIR "${COMPILER_CC}" DIRECTORY)
get_filename_component(CMAKE_FIND_ROOT_PATH "${COMPILER_DIR}" DIRECTORY)

# Look for includes and libraries only in the target system prefix.
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

# Helper variables to abstracts some common compiler flags.
set(COMPILER_NO_WARNINGS "-w" CACHE INTERNAL "")
@endcode

Please change the content accordingly.
