/*
 * Copyright (c) 2021-2023 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SUSPEND_DEBUG_S6_H__
#define __SUSPEND_DEBUG_S6_H__

#if BL30_SUSPEND_DEBUG_EN
extern struct xPwmMesonVoltage vddee_table[];
extern struct xPwmMesonVoltage vddcpu_table[];
extern struct xPwmMesonVoltage vdd_ddr_table[];

static inline void show_pwm_regs(void)
{
	uint32_t duty_reg, vol, table_size;

	if (IS_EN(BL30_SHOW_PWM_VOLT)) {
		// printf("PADCTRL_PIN_MUX_REGD 0x%x\n", REG32(PADCTRL_PIN_MUX_REGD));
		printf("PADCTRL_GPIOE_DS 0x%x\n", REG32(PADCTRL_GPIOE_DS));
		printf("CLKCTRL_PWM_CLK_CD_CTRL 0x%x\n", REG32(CLKCTRL_PWM_CLK_CD_CTRL));
		printf("CLKCTRL_PWM_CLK_EF_CTRL 0x%x\n", REG32(CLKCTRL_PWM_CLK_EF_CTRL));
		printf("CLKCTRL_PWM_CLK_AB_CTRL 0x%x\n", REG32(CLKCTRL_PWM_CLK_AB_CTRL));
		printf("PWM_MISC_REG_D 0x%x\n", REG32(PWM_MISC_REG_D));//EE
		printf("PWM_MISC_REG_F 0x%x\n", REG32(PWM_MISC_REG_F));//CPU
		printf("PWM_MISC_REG_A 0x%x\n", REG32(PWM_MISC_REG_A));//DDR

		duty_reg = REG32(PWM_PWM_D);
		table_size = vPwmMesonGetVoltTableSize(VDDEE_VOLT);
		for (int i = 0; i < table_size; i++)
			if (duty_reg == vddee_table[i].Duty_reg) {
				vol = vddee_table[i].Voltage_mv;
				break;
			}
		printf("PWM_PWM_D 0x%x, vddee vol %d mV\n", duty_reg, vol);

		duty_reg = REG32(PWM_PWM_A);
		table_size = vPwmMesonGetVoltTableSize(VDDDDR_VOLT);
		for (int i = 0; i < table_size; i++)
			if (duty_reg == vdd_ddr_table[i].Duty_reg) {
				vol = vdd_ddr_table[i].Voltage_mv;
				break;
			}
		printf("PWM_PWM_A 0x%x, vddddr vol %d mV\n", duty_reg, vol);

		if (xGpioGetValue(GPIO_TEST_N) != GPIO_LEVEL_LOW) {
			duty_reg = REG32(PWM_PWM_F);
			table_size = vPwmMesonGetVoltTableSize(VDDCPU_VOLT);
			for (int i = 0; i < table_size; i++)
				if (duty_reg == vddcpu_table[i].Duty_reg) {
					vol = vddcpu_table[i].Voltage_mv;
					break;
				}
			printf("PWM_PWM_F 0x%x, vddcpu vol %d mV\n", duty_reg, vol);
		} else
			printf("vddcpu is poweroff\n");
	} //BL30_SHOW_PWM_VOLT
}
static const char * const cpu_str[] = {
	[0] = "CPU_TOP",
	[1] = "DSPA",
};

static inline void dump_cpuN_fsm_regs(uint32_t i)
{
	printf("%s_FSM\n", cpu_str[i]);
	for (int j = 0; j < 0x30; j++)
		printf("0x%x\n ", REG32(0x30*i + PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 + 4 * j));
}

static inline void dump_cpu_fsm_regs(void)
{
	/* for s6, have cpu_top and DSPA*/
	if (IS_EN(BL30_DUMP_CPU_FSM)) {
		for (int id = 0; id < 2; id++)
			dump_cpuN_fsm_regs(id);
	}
}
#endif

#endif /* __SUSPEND_DEBUG_S6_H__ */
