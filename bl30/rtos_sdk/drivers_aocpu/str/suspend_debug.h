/*
 * Copyright (c) 2021-2023 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SUSPEND_DEBUG_H__
#define __SUSPEND_DEBUG_H__

#include <stdio.h>
#include "FreeRTOS.h" /* Must come first. */
#include "task.h" /* RTOS task related API prototypes. */
#include "common.h"

#ifndef CONFIG_BRINGUP
#define BL30_SUSPEND_DEBUG_EN 1
#endif
#define POWER_MODE_MASK 0xF

#if BL30_SUSPEND_DEBUG_EN

#define BL30_START_DEBUG_TASK (1U << (20))
#define BL30_SKIP_DDR_SUSPEND (1U << (21))
#define BL30_SKIP_POWER_SWITCH (1U << (22))
//#define BL30_DO_DDR_ACCESS (1U << (23))
#define BL30_DUMP_CPU_FSM (1U << (24))
#define BL30_OPEN_DMC_MONITOR_LOG (1U << (25))
#define BL30_SHOW_FUNC_LOG (1U << (26))
#define BL30_SHOW_PWM_VOLT (1U << (27))
#define BL30_IR_WAKEUP_MASK (1U << (28))
#define BL30_SARADC_WAKEUP_MASK (1U << (29))
#define BL30_RTC_WAKEUP_MASK (1U << (30))
//#define BL30_PD_CLK_GATE (1U << (31))
#define SUSPEND_DEBUG_MASK 0xFFF00000

#define TEST_TASK1_DELAY		(1000)	// ms

void split_suspend_flag(uint32_t *temp);
uint32_t get_suspend_flag(void);
extern uint32_t suspend_debug_flag;
#define IS_EN(x) (suspend_debug_flag & x)

#ifdef SYSCTRL_TIMERE
#define TIMERE_ADDR SYSCTRL_TIMERE
#elif defined(ISA_TIMERE)
#define TIMERE_ADDR ISA_TIMERE
#endif

/* show function call log */
#define enter_func_print() do { \
	if (IS_EN(BL30_SHOW_FUNC_LOG)) \
		printf("enter %s line %d,TE: %ud\n", \
			__func__, __LINE__, REG32(TIMERE_ADDR)); \
} while (0)

#define exit_func_print() do { \
	if (IS_EN(BL30_SHOW_FUNC_LOG)) \
		printf("exit %s line %d,TE: %ud\n", \
			__func__, __LINE__, REG32(TIMERE_ADDR)); \
} while (0)

extern void vDebugPrintTask(void *pvParameters);

#define start_debug_task() do { \
	if (IS_EN(BL30_START_DEBUG_TASK))\
		xTaskCreate(vDebugPrintTask, "Print_task", \
			configMINIMAL_STACK_SIZE, NULL, 2, &printTask); \
} while (0)

#define stop_debug_task() do { \
	if (IS_EN(BL30_START_DEBUG_TASK))\
		vTaskDelete(printTask); \
} while (0)

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

#endif

#endif /* __SUSPEND_DEBUG_H__ */

