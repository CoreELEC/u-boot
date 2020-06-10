// See LICENSE for license details.

// See LICENSE for license details.

#include "riscv_encoding.h"

	.section .vtable

	.weak  eclic_msip_handler

	.weak  eclic_mtip_handler

	.weak  eclic_meip_handler

	.weak  eclic_irq19_handler

	.weak  eclic_irq20_handler
	.weak  eclic_irq21_handler
	.weak  eclic_irq22_handler
	.weak  eclic_irq23_handler

	.weak  eclic_irq24_handler
	.weak  eclic_irq25_handler
	.weak  eclic_irq26_handler
	.weak  eclic_irq27_handler

	.weak  eclic_irq28_handler
	.weak  eclic_irq29_handler
	.weak  eclic_irq30_handler
	.weak  eclic_irq31_handler

	.weak  eclic_irq32_handler
	.weak  eclic_irq33_handler
	.weak  eclic_irq34_handler
	.weak  eclic_irq35_handler

	.weak  eclic_irq36_handler
	.weak  eclic_irq37_handler
	.weak  eclic_irq38_handler
	.weak  eclic_irq39_handler

	.weak  eclic_irq40_handler
	.weak  eclic_irq41_handler
	.weak  eclic_irq42_handler
	.weak  eclic_irq43_handler

	.weak  eclic_irq44_handler
	.weak  eclic_irq45_handler
	.weak  eclic_irq46_handler
	.weak  eclic_irq47_handler

	.weak  eclic_irq48_handler
	.weak  eclic_irq49_handler
	.weak  eclic_irq50_handler
	.weak  eclic_irq51_handler

	.weak  eclic_irq52_handler
	.weak  eclic_irq53_handler
	.weak  eclic_irq54_handler
	.weak  eclic_irq55_handler

	.weak  eclic_irq56_handler
	.weak  eclic_irq57_handler
	.weak  eclic_irq58_handler
	.weak  eclic_irq59_handler

	.weak  eclic_irq60_handler
	.weak  eclic_irq61_handler
	.weak  eclic_irq62_handler
	.weak  eclic_irq63_handler

    .globl vector_base
vector_base:
	 j _start
	.word  0
	.word  0
	.word  eclic_msip_handler

	.word  0
	.word  0
	.word  0
	.word  eclic_mtip_handler

	.word  0
	.word  0
	.word  0
	.word  0

	.word  0
	.word  0
	.word  0
	.word  0

	.word  0
	.word  0
	.word  0
	.word  eclic_irq19_handler

	.word  eclic_irq20_handler
	.word  eclic_irq21_handler
	.word  eclic_irq22_handler
	.word  eclic_irq23_handler

	.word  eclic_irq24_handler
	.word  eclic_irq25_handler
	.word  eclic_irq26_handler
	.word  eclic_irq27_handler

	.word  eclic_irq28_handler
	.word  eclic_irq29_handler
	.word  eclic_irq30_handler
	.word  eclic_irq31_handler

	.word  eclic_irq32_handler
	.word  eclic_irq33_handler
	.word  eclic_irq34_handler
	.word  eclic_irq35_handler

	.word  eclic_irq36_handler
	.word  eclic_irq37_handler
	.word  eclic_irq38_handler
	.word  eclic_irq39_handler

	.word  eclic_irq40_handler
	.word  eclic_irq41_handler
	.word  eclic_irq42_handler
	.word  eclic_irq43_handler

	.word  eclic_irq44_handler
	.word  eclic_irq45_handler
	.word  eclic_irq46_handler
	.word  eclic_irq47_handler

	.word  eclic_irq48_handler
	.word  eclic_irq49_handler
	.word  eclic_irq50_handler
	.word  eclic_irq51_handler

	.word  eclic_irq52_handler
	.word  eclic_irq53_handler
	.word  eclic_irq54_handler
	.word  eclic_irq55_handler

	.word  eclic_irq56_handler
	.word  eclic_irq57_handler
	.word  eclic_irq58_handler
	.word  eclic_irq59_handler

	.word  eclic_irq60_handler
	.word  eclic_irq61_handler
	.word  eclic_irq62_handler
	.word  eclic_irq63_handler


