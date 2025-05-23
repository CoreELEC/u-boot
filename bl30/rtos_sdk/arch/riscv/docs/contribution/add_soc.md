How to Add a New SoC	{#add_soc}
==========

Let's take ***a4*** for example.

### Step 1: Prepare your source code. ###
Make directory ***soc/riscv/a4***, and put your source code into it.

	mkdir -p soc/riscv/a4

### Step 2: Add ***Kconfig***. ###
Write ***Kconfig*** and put it into ***soc/riscv/a4***.

@code
config SOC_A4
	bool "A4"
	select RISCV
	help
	  Enable A4 SOC of RISCV.
...
@endcode

Please change the content accordingly.

### Step 3: Add ***CMakeLists.txt***. ###
Write ***CMakeLists.txt*** and put it into ***soc/riscv/a4***.

@code
aml_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

aml_library_sources(
	...
)

@endcode

Please change the include directories, source code and dependent libraries accordingly.
