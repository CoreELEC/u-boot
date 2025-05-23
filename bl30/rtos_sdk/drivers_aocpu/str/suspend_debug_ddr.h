/*
 * Copyright (c) 2021-2023 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SUSPEND_DEBUG_DDR_H__
#define __SUSPEND_DEBUG_DDR_H__

#include "suspend_debug.h"

#if BL30_SUSPEND_DEBUG_EN

#define show_dmc_port_status() do { \
	if (IS_EN(BL30_OPEN_DMC_MONITOR_LOG)) {                       \
		printf("DMC_CHAN_STS: 0x%x\n", rd_reg(DMC_CHAN_STS));     \
		printf("DMC_PROT_VIO_0: 0x%x\n", rd_reg(DMC_PROT_VIO_0)); \
		printf("DMC_PROT_VIO_1: 0x%x\n", rd_reg(DMC_PROT_VIO_1)); \
		printf("DMC_PROT_VIO_2: 0x%x\n", rd_reg(DMC_PROT_VIO_2)); \
		printf("DMC_PROT_VIO_3: 0x%x\n", rd_reg(DMC_PROT_VIO_3)); \
	}								  \
} while (0)

#define dmc_status_print_clear() do {       \
	if (IS_EN(BL30_OPEN_DMC_MONITOR_LOG)) { \
		show_dmc_port_status();             \
		wr_reg(DMC_PROT_IRQ_CTRL, 0x3);     \
	}										\
} while (0)

#define dmc_status_disable_print() do {     \
	if (IS_EN(BL30_OPEN_DMC_MONITOR_LOG)) { \
		wr_reg(DMC_PROT_IRQ_CTRL, 0x3);     \
		show_dmc_port_status();             \
	}										\
} while (0)

#endif // BL30_SUSPEND_DEBUG_EN

#endif /* __SUSPEND_DEBUG_DDR_H__ */

