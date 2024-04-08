/*
 * Copyright (c) 2021-2023 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SUSPEND_DEBUG_T6D_H__
#define __SUSPEND_DEBUG_T6D_H__

#include "suspend_debug.h"

#if BL30_SUSPEND_DEBUG_EN
extern struct xPwmMesonVoltage vddee_table[];
extern struct xPwmMesonVoltage vddcpu_table[];
static inline void show_pwm_regs(void)
{
	uint32_t temp_val, vol, table_size;

	if (IS_EN(BL30_SHOW_PWM_VOLT)) {
		printf("PADCTRL_PIN_MUX_REGD 0x%x\n", REG32(PADCTRL_PIN_MUX_REGD));
		printf("PADCTRL_GPIOE_DS 0x%x\n", REG32(PADCTRL_GPIOE_DS));
		printf("CLKCTRL_PWM_CLK_AB_CTRL 0x%x\n", REG32(CLKCTRL_PWM_CLK_AB_CTRL));
		printf("PWMAB_MISC_REG_AB 0x%x\n", REG32(PWMAB_MISC_REG_AB));
		temp_val = REG32(PWMAB_PWM_A);
		table_size = vPwmMesonGetVoltTableSize(VDDEE_VOLT);
		for (int i = 0; i < table_size; i++)
			if (temp_val == vddee_table[i].Duty_reg) {
				vol = vddee_table[i].Voltage_mv;
				break;
			}
		printf("PWMAB_PWM_A 0x%x,vddee vol %d mV\n", temp_val, vol);
		if (REG32(PWMEF_MISC_REG_AB) & (1 << 1)) {
			temp_val = REG32(PWMAB_PWM_B);
			table_size = vPwmMesonGetVoltTableSize(VDDCPU_VOLT);
			for (int i = 0; i < table_size; i++)
				if (temp_val == vddcpu_table[i].Duty_reg) {
					vol = vddcpu_table[i].Voltage_mv;
					break;
				}
			printf("PWMAB_PWM_B 0x%x,vddcpu %d mV\n", temp_val, vol);
		} else
			printf("vddcpu is poweroff\n");
	}
}
static const char * const cpu_str[] = {
	[0] = "CPU0",
	[1] = "CPU1",
	[2] = "CPU2",
	[3] = "CPU3",
	[4] = "CPU4",
	[5] = "CPU_TOP",
};

static inline void dump_cpuN_fsm_regs(uint32_t i)
{
	printf("%s_FSM\n", cpu_str[i]);
	for (int j = 0; j < 0x30; j++)
		printf("0x%x\n", REG32(0x30*i + PWRCTRL_CPU0_FSM_START_ON + 4 * j));
}

static inline void dump_cpu_fsm_regs(void)
{
	/* for t6d,dump fsm regs */
	if (IS_EN(BL30_DUMP_CPU_FSM)) {
		for (int id = 0; id < 6; id++)
			dump_cpuN_fsm_regs(id);
	}
}
#endif

#endif /* __SUSPEND_DEBUG_T6D_H__ */
