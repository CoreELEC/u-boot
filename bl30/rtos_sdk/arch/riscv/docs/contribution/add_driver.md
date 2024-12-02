How to Add a New Driver	{#add_driver}
==========

Let's take ***uart*** driver for example.

### Step 1: Prepare your source code. ###
Make directory ***drivers_aocpu/uart***, and put your source code into it.

	mkdir -p drivers_aocpu/uart

### Step 2: Add ***Kconfig***. ###
Write ***Kconfig*** for your driver.

@code
config UART
	bool "UART Driver"

if UART
config UART_WAKEUP
	bool "UART Wake up Function"
	help
	    help select uart wake up function setting.

endif # UART
@endcode

Then add the following line to ***drivers_aocpu/Kconfig***.
@code
rsource "uart/Kconfig"
@endcode

### Step 3: Add ***CMakeLists.txt***. ###
Write ***CMakeLists.txt*** for your driver.

@code
aml_library_include_directories(
	${CMAKE_CURRENT_LIST_DIR}
)

aml_library_sources_ifdef(
	CONFIG_UART
	uart.c
)
@endcode

Then add the following line to ***drivers_aocpu/CMakeLists.txt***.
@code
...
add_subdirectory_if_kconfig(uart)
...

aml_library_include_directories(
	...
	${SDK_BASE}/drivers_aocpu/uart
)
@endcode

### Step 4: Enable the new driver by default. ###
Let's take ***boards/riscv/ba400_a113l2*** for example.

Add the following line to ***boards/riscv/ba400_a113l2/defconfig***.

@code
CONFIG_UART=y
@endcode

### Step 5: Initialize your driver ###
Add driver startup code to ***boards/riscv/ba400_a113l2/hw_business.c***

```c

void hw_business_process(void)
{
	...
	uart_init();
	...
}

```
