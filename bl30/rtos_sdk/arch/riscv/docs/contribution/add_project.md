How to Add a New Project	{#add_project}
==========

A project is a build combination of **hardware architecture**, **SoC**, **board** and **product**.\n
So you need to add them first.

- @subpage add_arch
- @subpage add_soc
- @subpage add_board
- @subpage add_product

Then you can add your new project.The format of the message line need follow:

	arch_dir_name soc_dir_name board_dir_name(chip_name+_+package_name) project_dir_name

Let's take ARCH riscv, SOC a4, BOARD ba400_a113l2, PRODUCT aocpu for example.

Please add the following line to ***boards/$ARCH/build_combination.in***.

	riscv a4 ba400_a113l2 aocpu

Q&A

- ***source script/env.sh*** and you can't find your project line after you do all the steps:***rm -rf output/build_combination.txt***,then source script/env.sh again
