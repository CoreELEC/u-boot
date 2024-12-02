How to Add a New Board	{#add_board}
==========

Let's take ***ba400_a113l2*** for example.

### Step 1: Prepare your source code. ###
Make directory ***boards/riscv/ba400_a113l2***, and put your source code into it.

	mkdir -p boards/riscv/ba400_a113l2

### Step 2: Add ***Kconfig***. ###
Write ***Kconfig*** and put it into ***boards/riscv/ba400_a113l2***.

@code
config BOARD_BA400_A113L2
	bool "Amlogic BA400 Board"
	select SOC_A4
	...
	help
	  Enable Amlogic BA400 Board.
...
@endcode

Please change the content accordingly.

### Step 3: Add ***CMakeLists.txt***. ###
Write ***CMakeLists.txt*** and put it into ***boards/riscv/ba400_a113l2***.

@code
aml_library_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

aml_library_sources(
	...
)
@endcode

Please change the include directories, source code and dependent libraries accordingly.

### Step 4: Add ***defconfig***. ###
Write ***defconfig*** and put it into ***boards/riscv/ba400_a113l2***.

@code
CONFIG_BOARD_BA400_A113L2=y
...
@endcode

Note that ***defconfig*** <span style="color:red">defines the default **hardware** features</span>.\n
Please change the features accordingly.
