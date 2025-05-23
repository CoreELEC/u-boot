Important Environment Variables {#env_variables}
==========

There are some important environment variables set by ***scripts/env***, which are essential to the build system.

	ARCH

The build system chooses the corresponding ***architecture*** directory based on this variable.

	SOC

The build system chooses the corresponding ***SoC*** directory based on this variable.

	BOARD

The build system chooses the corresponding ***board*** directory based on this variable.

	PRODUCT

The build system chooses the corresponding ***product*** directory based on this variable.

You coud use these environment variables when writing ***Kconfig*** and ***CMakeLists.txt***.\n
Here is a Kconfig example.
@code
source "arch/${ARCH}/Kconfig"
source "boards/${ARCH}/Kconfig"
source "soc/${ARCH}/Kconfig"
@endcode

Here is a CMake example.
@code
add_subdirectory(arch/${ARCH})
add_subdirectory(boards/${ARCH})
@endcode
