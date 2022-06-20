/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef EXTRA_REGISTER_H
#define EXTRA_REGISTER_H

#define AO_CECB_CLK_CNTL_REG0 (0xff800000 + (0x0a0 << 2))
#define SEC_AO_CECB_CLK_CNTL_REG0 (0xff800000 + (0x0a0 << 2))
#define P_AO_CECB_CLK_CNTL_REG0 ((volatile uint32_t *)(0xff800000 + (0x0a0 << 2)))
#define AO_CECB_CLK_CNTL_REG1 (0xff800000 + (0x0a1 << 2))
#define SEC_AO_CECB_CLK_CNTL_REG1 (0xff800000 + (0x0a1 << 2))
#define P_AO_CECB_CLK_CNTL_REG1 ((volatile uint32_t *)(0xff800000 + (0x0a1 << 2)))

#define UART1_WFIFO ((0x8c00 << 2) + 0xffd00000)
#define AO_GPIO_TEST_N (0xff800000 + (0x0d7 << 2))
#define AO_SAR_ADC_REG0 (0xff809000 + (0x000 << 2))
#define AO_SAR_CLK (0xff800000 + (0x024 << 2))
#define AO_SAR_ADC_REG0 (0xff809000 + (0x000 << 2))
#define AO_RTI_STICKY_REG2 (0xff800000 + (0x04e << 2))

#define AO_GPIO_O_EN_N (0xff800000 + (0x009 << 2))
#define AO_RTI_PINMUX_REG0 (0xff800000 + (0x005 << 2))
#define AO_PAD_DS_A (0xff800000 + (0x007 << 2))
#define PAD_PULL_UP_EN_REG0 (0xff634400 + (0x048 << 2))
#define PAD_PULL_UP_REG0 (0xff634400 + (0x03a << 2))
#define PREG_PAD_GPIO0_EN_N (0xff634400 + (0x010 << 2))
#define PERIPHS_PIN_MUX_0 (0xff634400 + (0x0b0 << 2))
#define PAD_DS_REG0A (0xff634400 + (0x0d0 << 2))
#define AO_IRQ_GPIO_REG (0xff800000 + (0x021 << 2))

#define AO_CECB_GEN_CNTL (0xff800000 + (0x0a2 << 2))
#define AO_CECB_RW_REG (0xff800000 + (0x0a3 << 2))
#define AO_CECB_INTR_MASKN (0xff800000 + (0x0a4 << 2))
#define AO_CECB_INTR_CLR (0xff800000 + (0x0a5 << 2))
#define AO_CECB_INTR_STAT (0xff800000 + (0x0a6 << 2))
#define AO_DEBUG_REG0 (0xff800000 + (0x028 << 2))
#define AO_DEBUG_REG1 (0xff800000 + (0x029 << 2))

#define AM_DDR_PLL_CNTL0 ((0x0000 << 2) + 0xff638c00)
#define AM_DDR_PLL_CNTL1 ((0x0001 << 2) + 0xff638c00)
#define AM_DDR_PLL_CNTL2 ((0x0002 << 2) + 0xff638c00)
#define AM_DDR_PLL_CNTL3 ((0x0003 << 2) + 0xff638c00)
#define AM_DDR_PLL_CNTL5 ((0x0005 << 2) + 0xff638c00)
#define AM_DDR_PLL_CNTL6 ((0x0006 << 2) + 0xff638c00)

#define PWM_PWM_A ((0x6c00 << 2) + 0xffd00000)
#define PWM_PWM_C ((0x6800 << 2) + 0xffd00000)
#define PWM_PWM_E ((0x6400 << 2) + 0xffd00000)

#define ISA_TIMERB ((0x3c52 << 2) + 0xffd00000)
#define ISA_TIMERE_HI ((0x3c63 << 2) + 0xffd00000)
#define ISA_SOFT_IRQ ((0x3c90 << 2) + 0xffd00000)
#define ISA_TIMERE ((0x3c62 << 2) + 0xffd00000)
#define ISA_TIMER_MUX1 ((0x3c64 << 2) + 0xffd00000)
#define SEC_AO_WATCHDOG_CNTL (0xff800000 + (0x048 << 2))
#define PREG_STICKY_REG3 (0xff634400 + (0x073 << 2))
#define ISA_TIMER_MUX ((0x3c50 << 2) + 0xffd00000)

#define SEC_SYS_CPU_CFG10 (0xff634400 + (0x09c << 2))

#define AO_DEBUG_REG2 (0xff800000 + (0x02a << 2))

#define AO_CEC_STICKY_DATA0 (0xff800000 + (0x0ca << 2))
#define SEC_AO_CEC_STICKY_DATA0 (0xff800000 + (0x0ca << 2))
#define P_AO_CEC_STICKY_DATA0 ((volatile uint32_t *)(0xff800000 + (0x0ca << 2)))
#define AO_CEC_STICKY_DATA1 (0xff800000 + (0x0cb << 2))
#define SEC_AO_CEC_STICKY_DATA1 (0xff800000 + (0x0cb << 2))
#define P_AO_CEC_STICKY_DATA1 ((volatile uint32_t *)(0xff800000 + (0x0cb << 2)))
#define AO_CEC_STICKY_DATA2 (0xff800000 + (0x0cc << 2))
#define SEC_AO_CEC_STICKY_DATA2 (0xff800000 + (0x0cc << 2))
#define P_AO_CEC_STICKY_DATA2 ((volatile uint32_t *)(0xff800000 + (0x0cc << 2)))
#define AO_CEC_STICKY_DATA3 (0xff800000 + (0x0cd << 2))
#define SEC_AO_CEC_STICKY_DATA3 (0xff800000 + (0x0cd << 2))
#define P_AO_CEC_STICKY_DATA3 ((volatile uint32_t *)(0xff800000 + (0x0cd << 2)))
#define AO_CEC_STICKY_DATA4 (0xff800000 + (0x0ce << 2))
#define SEC_AO_CEC_STICKY_DATA4 (0xff800000 + (0x0ce << 2))
#define P_AO_CEC_STICKY_DATA4 ((volatile uint32_t *)(0xff800000 + (0x0ce << 2)))
#define AO_CEC_STICKY_DATA5 (0xff800000 + (0x0cf << 2))
#define SEC_AO_CEC_STICKY_DATA5 (0xff800000 + (0x0cf << 2))
#define P_AO_CEC_STICKY_DATA5 ((volatile uint32_t *)(0xff800000 + (0x0cf << 2)))
#define AO_CEC_STICKY_DATA6 (0xff800000 + (0x0d0 << 2))
#define SEC_AO_CEC_STICKY_DATA6 (0xff800000 + (0x0d0 << 2))
#define P_AO_CEC_STICKY_DATA6 ((volatile uint32_t *)(0xff800000 + (0x0d0 << 2)))
#define AO_CEC_STICKY_DATA7 (0xff800000 + (0x0d1 << 2))
#define SEC_AO_CEC_STICKY_DATA7 (0xff800000 + (0x0d1 << 2))
#define P_AO_CEC_STICKY_DATA7 ((volatile uint32_t *)(0xff800000 + (0x0d1 << 2)))


#define RTC_INT_MASK ((0x0008 << 2) + 0xfe09a000)
#define RTC_INT_CLR ((0x0009 << 2) + 0xfe09a000)
#define RTC_REAL_TIME ((0x000d << 2) + 0xfe09a000)
#define RTC_INT_STATUS ((0x000c << 2) + 0xfe09a000)
#define RTC_ALARM0_REG ((0x0002 << 2) + 0xfe09a000)

#endif // EXTRA_REGISTER_H

