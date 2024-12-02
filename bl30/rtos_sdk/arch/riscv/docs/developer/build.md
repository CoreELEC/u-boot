How to Build SDK {#build}
===============

## [Install Dependencies](sys_requirements.html) ##

## Choose your project ##

The project is a combination of<span style="color:red"> **ARCH**, **SOC**, **BOARD**,</span> and<span style="color:red"> **PRODUCT**.</span>\n
If you already know the these parameters, you could specify them by the command line parameters.

	source scripts/env.sh [ARCH] [SOC] [BOARD] [PRODUCT]

For ARCH *riscv*, SOC *a4*, BOARD *ba400_a113l2*, PRODUCT *aocpu*, you could type the following command.\n

	source scripts/env.sh riscv a4 ba400_a113l2 aocpu

Otherwise, you could choose one project from the list.\n

	source scripts/env.sh

## Build ##

To build a specific product for a specific board, use the following command:

	make

Note that the building environment will be extracted automatically for the first time running.\n
After the compilation, the images can be found in the directory ***output/${ARCH}-${BOARD}-${PRODUCT}/images***.

For ARCH *riscv*, SOC *a4*, BOARD *ba400_a113l2*, PRODUCT *aocpu*,
the images can be found in the directory ***output/riscv-ba400_a113l2-aocpu/images***.

---
### Menuconfig ###
To adjust the default configuration, please use menuconfig.

	make menuconfig

### Build Document ###

	make docs DOCS_ARCH=riscv

To view the local document, please open ***output/docs/riscv/html/index.html*** with any web browser.
