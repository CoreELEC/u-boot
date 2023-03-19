/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "task.h" /* RTOS task related API prototypes. */
#include <common.h>
#include "power_domain.h"
#include "p_register.h"
#include "timer_source.h"

int is_domain_power_on(enum PM domain)
{
	int ret = 0;
	int id = (int)domain;
	uint8_t fsm;

	switch (domain) {
	case PM_CPU_PWR:
		fsm = (((*P_PWRCTRL_CPUTOP_FSM_STS0) >> 12) & 0x1f);
		ret = (fsm == PWR_STATE_IDLE) ? 1 : ((fsm == PWR_STATE_WAIT_ON) ? 0 : -1);
		break;
	case PM_CPU_CORE0:
		fsm = ((*P_PWRCTRL_CPU0_FSM_STS0 >> 12) & 0x1f);
		ret = (fsm == PWR_STATE_IDLE) ? 1 : ((fsm == PWR_STATE_WAIT_ON) ? 0 : -1);
		break;
	case PM_CPU_CORE1:
		fsm = ((*P_PWRCTRL_CPU1_FSM_STS0 >> 12) & 0x1f);
		ret = (fsm == PWR_STATE_IDLE) ? 1 : ((fsm == PWR_STATE_WAIT_ON) ? 0 : -1);
		break;
	case PM_CPU_CORE2:
		fsm = ((*P_PWRCTRL_CPU2_FSM_STS0 >> 12) & 0x1f);
		ret = (fsm == PWR_STATE_IDLE) ? 1 : ((fsm == PWR_STATE_WAIT_ON) ? 0 : -1);
		break;
	case PM_CPU_CORE3:
		fsm = ((*P_PWRCTRL_CPU3_FSM_STS0 >> 12) & 0x1f);
		ret = (fsm == PWR_STATE_IDLE) ? 1 : ((fsm == PWR_STATE_WAIT_ON) ? 0 : -1);
		break;
	case PM_AOCPU:
		fsm = ((*P_PWRCTRL_AOCPU_FSM_STS0 >> 12) & 0x1f);
		ret = (fsm == PWR_STATE_IDLE) ? 1 : ((fsm == PWR_STATE_WAIT_ON) ? 0 : -1);
		break;
	default:
		if (id < 31) {
			ret = (*P_PWRCTRL_FOCRST0 & (1 << id)) == 0 ?
				      1 :
				      (((*P_PWRCTRL_PWR_OFF0 & (1 << id)) != 0) ? 0 : -1);
		} else if (id < 64) {
			ret = (*P_PWRCTRL_FOCRST1 & (1<<(id-32))) == 0 ?
					1 :
					(((*P_PWRCTRL_PWR_OFF0 & (1<<(id-32))) != 0) ? 0 : -1);
		} else {
			ret = (*P_PWRCTRL_FOCRST1 & (1 << (id-64))) == 0 ?
				      1 :
				      (((*P_PWRCTRL_PWR_OFF1 & (1 << (id-64))) != 0) ? 0 : -1);
		}
		break;
	}

	/* 0: OFF, 1: ON, -1: switching */
	return ret;
}

int get_domain_id(enum PM domain)
{
	int id = 0;

	id = (int)domain;

	return id;
}

void do_power_switch(enum PM domain, uint32_t pwr_state)
{
	int id = get_domain_id(domain);

	if (pwr_state == PWR_OFF) {
		switch (domain) {
		case PM_CPU_PWR:
			*P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 |= (1 << 8);
			break;
		case PM_CPU_CORE0:
			*P_PWRCTRL_CPU0_AUTO_OFF_CTRL0 |= (1 << 8);
			break;
		case PM_CPU_CORE1:
			*P_PWRCTRL_CPU1_AUTO_OFF_CTRL0 |= (1 << 8);
			break;
		case PM_CPU_CORE2:
			*P_PWRCTRL_CPU2_AUTO_OFF_CTRL0 |= (1 << 8);
			break;
		case PM_CPU_CORE3:
			*P_PWRCTRL_CPU3_AUTO_OFF_CTRL0 |= (1 << 8);
			break;
		case PM_DSPA:
			*P_PWRCTRL_DSPA_AUTO_OFF_CTRL0 |= (1 << 8);
			break;
		case PM_AOCPU:
			*P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0 |= (1 << 8);
			break;
		default:
			if (id < 32)
				*P_PWRCTRL_PWR_OFF0 |= (1 << id);
			else
				*P_PWRCTRL_PWR_OFF1 |= (1 << (id - 32));
			break;
		}
	} else {
		switch (domain) {
		case PM_CPU_PWR:
			*P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 &= ~(1 << 8);
			break;
		case PM_CPU_CORE0:
			*P_PWRCTRL_CPU0_AUTO_OFF_CTRL0 &= ~(1 << 8);
			break;
		case PM_CPU_CORE1:
			*P_PWRCTRL_CPU1_AUTO_OFF_CTRL0 &= ~(1 << 8);
			break;
		case PM_CPU_CORE2:
			*P_PWRCTRL_CPU2_AUTO_OFF_CTRL0 &= ~(1 << 8);
			break;
		case PM_CPU_CORE3:
			*P_PWRCTRL_CPU3_AUTO_OFF_CTRL0 &= ~(1 << 8);
			break;
		case PM_DSPA:
			*P_PWRCTRL_DSPA_AUTO_OFF_CTRL0 &= ~(1 << 8);
			break;
		case PM_AOCPU:
			*P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0 &= ~(1 << 8);
			break;
		default:
			if (id < 32)
				*P_PWRCTRL_PWR_OFF0 &= ~(1 << id);
			else
				*P_PWRCTRL_PWR_OFF1 &= ~(1 << (id - 32));
			break;
		}
	}
}

void do_power_memory(enum PM domain, uint32_t pwr_state)
{
	if (pwr_state == PWR_OFF) {
		switch (domain) {
		case PM_CPU_PWR:
			*P_PWRCTRL_CPUTOP_MEMPD_INIT_SET = 0xfffff;
			break;
		case PM_CPU_CORE0:
			*P_PWRCTRL_CPU0_MEMPD_INIT_SET = 0x3ffffff;
			break;
		case PM_CPU_CORE1:
			*P_PWRCTRL_CPU1_MEMPD_INIT_SET = 0x3ffffff;
			break;
		case PM_CPU_CORE2:
			*P_PWRCTRL_CPU2_MEMPD_INIT_SET = 0x3ffffff;
			break;
		case PM_CPU_CORE3:
			*P_PWRCTRL_CPU3_MEMPD_INIT_SET = 0x3ffffff;
			break;
		case PM_DSPA:
			*P_PWRCTRL_DSPA_MEMPD_INIT_SET = 0xffff;
			break;
		case PM_AOCPU:
			break;
		case PM_NNA:
			*P_PWRCTRL_MEM_PD0 = 0xffffffff;
			*P_PWRCTRL_MEM_PD1 = 0xffffffff;
			break;
		case PM_AUDIO:
			*P_PWRCTRL_MEM_PD4 |= 0xffff3cff;
			break;
		case PM_SDIOA:
			*P_PWRCTRL_MEM_PD5 |= (0x3 << 8);
			break;
		case PM_EMMC:
			*P_PWRCTRL_MEM_PD5 |= (0x3 << 10);
			break;
		case PM_USB_COMB:
			*P_PWRCTRL_MEM_PD11 |= (0X3 << 10);
			break;
		case PM_SYS_WRAP:
			*P_PWRCTRL_MEM_PD4 |= (0x3 << 8 | 0x3 << 14);
			break; // ROM & KL
		case PM_ETH:
			*P_PWRCTRL_MEM_PD11 |= (0x3 << 8);
			break;
		case PM_RSA:
			*P_PWRCTRL_MEM_PD5 |= (0x3 << 16);
			break;
		case PM_AUDIO_PDM:
			*P_PWRCTRL_MEM_PD5 |= (0x3 << 4);
			break;
		case PM_DMC:
			*P_PWRCTRL_MEM_PD5 |= (0x3 << 6);
			break;
		default:
			break;
		}
	} else {
		switch (domain) {
		case PM_CPU_PWR:
			*P_PWRCTRL_CPUTOP_MEMPD_INIT_SET = 0x0;
			break;
		case PM_CPU_CORE0:
			*P_PWRCTRL_CPU0_MEMPD_INIT_SET = 0x0;
			break;
		case PM_CPU_CORE1:
			*P_PWRCTRL_CPU1_MEMPD_INIT_SET = 0x0;
			break;
		case PM_CPU_CORE2:
			*P_PWRCTRL_CPU2_MEMPD_INIT_SET = 0x0;
			break;
		case PM_CPU_CORE3:
			*P_PWRCTRL_CPU3_MEMPD_INIT_SET = 0x0;
			break;
		case PM_DSPA:
			*P_PWRCTRL_DSPA_MEMPD_INIT_SET = 0x0;
			break;
		case PM_AOCPU:
			break;
		case PM_NNA:
			*P_PWRCTRL_MEM_PD0 = 0x0;
			*P_PWRCTRL_MEM_PD1 = 0x0;
			break;
		case PM_AUDIO:
			*P_PWRCTRL_MEM_PD4 &= ~0xffff3cff;
			break;
		case PM_SDIOA:
			*P_PWRCTRL_MEM_PD5 &= ~(0x3 << 8);
			break;
		case PM_EMMC:
			*P_PWRCTRL_MEM_PD5 &= ~(0x3 << 10);
			break;
		case PM_USB_COMB:
			*P_PWRCTRL_MEM_PD11 &= ~(0X3 << 10);
			break;
		case PM_SYS_WRAP:
			*P_PWRCTRL_MEM_PD4 &= ~(0x3 << 8 | 0x3 << 14);
			break; // ROM & KL
		case PM_ETH:
			*P_PWRCTRL_MEM_PD11 &= ~(0X3 << 8);
			break;
		case PM_RSA:
			*P_PWRCTRL_MEM_PD5 &= ~(0X3 << 16);
			break;
		case PM_AUDIO_PDM:
			*P_PWRCTRL_MEM_PD5 &= ~(0X3 << 4);
			break;
		case PM_DMC:
			*P_PWRCTRL_MEM_PD5 &= ~(0x3 << 6);
			break;
		default:
			break;
		}
	}
}

void do_iso_en(enum PM domain, uint32_t pwr_state)
{
	int id = get_domain_id(domain);

	if (pwr_state == PWR_OFF) {
		switch (domain) {
		case PM_CPU_PWR:
			*P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 |= (1 << 4);
			break;
		case PM_CPU_CORE0:
			*P_PWRCTRL_CPU0_AUTO_OFF_CTRL0 |= (1 << 4);
			break;
		case PM_CPU_CORE1:
			*P_PWRCTRL_CPU1_AUTO_OFF_CTRL0 |= (1 << 4);
			break;
		case PM_CPU_CORE2:
			*P_PWRCTRL_CPU2_AUTO_OFF_CTRL0 |= (1 << 4);
			break;
		case PM_CPU_CORE3:
			*P_PWRCTRL_CPU3_AUTO_OFF_CTRL0 |= (1 << 4);
			break;
		case PM_DSPA:
			*P_PWRCTRL_DSPA_AUTO_OFF_CTRL0 |= (1 << 4);
			break;
		case PM_AOCPU:
			*P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0 |= (1 << 4);
			break;
		default:
			if (id < 32)
				*P_PWRCTRL_ISO_EN0 |= (1 << id);
			else
				*P_PWRCTRL_ISO_EN1 |= (1 << (id - 32));
			break;
		}
	} else {
		switch (domain) {
		case PM_CPU_PWR:
			*P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 &= ~(1 << 4);
			break;
		case PM_CPU_CORE0:
			*P_PWRCTRL_CPU0_AUTO_OFF_CTRL0 &= ~(1 << 4);
			break;
		case PM_CPU_CORE1:
			*P_PWRCTRL_CPU1_AUTO_OFF_CTRL0 &= ~(1 << 4);
			break;
		case PM_CPU_CORE2:
			*P_PWRCTRL_CPU2_AUTO_OFF_CTRL0 &= ~(1 << 4);
			break;
		case PM_CPU_CORE3:
			*P_PWRCTRL_CPU3_AUTO_OFF_CTRL0 &= ~(1 << 4);
			break;
		case PM_DSPA:
			*P_PWRCTRL_DSPA_AUTO_OFF_CTRL0 &= ~(1 << 4);
			break;
		//case PM_DSPB:       *P_PWRCTRL_DSPB_AUTO_OFF_CTRL0   &= ~(1<<4);  break;
		case PM_AOCPU:
			*P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0 &= ~(1 << 4);
			break;
		default:
			if (id < 32)
				*P_PWRCTRL_ISO_EN0 &= ~(1 << id);
			else
				*P_PWRCTRL_ISO_EN1 &= ~(1 << (id - 32));
			break;
		}
	}
}

char *get_domain_name(enum PM domain)
{
	char *domain_name = NULL;

	switch (domain) {
	case PM_CPU_PWR:
		domain_name = "CPU_PWR";
		break;
	case PM_CPU_CORE0:
		domain_name = "CPU_CORE0";
		break;
	case PM_CPU_CORE1:
		domain_name = "CPU_CORE1";
		break;
	case PM_CPU_CORE2:
		domain_name = "CPU_CORE2";
		break;
	case PM_CPU_CORE3:
		domain_name = "CPU_CORE3";
		break;
	case PM_DSPA:
		domain_name = "PM_DSPA";
		break;
	case PM_AOCPU:
		domain_name = "PM_AOCPU";
		break;
	case PM_NNA:
		domain_name = "PM_NNA";
		break;
	case PM_AUDIO:
		domain_name = "PM_AUDIO";
		break;
	case PM_RSA:
		domain_name = "PM_RSA";
		break;
	case PM_SDIOA:
		domain_name = "PM_SDIOA";
		break;
	case PM_EMMC:
		domain_name = "PM_EMMC";
		break;
	case PM_USB_COMB:
		domain_name = "PM_USB_COMB";
		break;
	case PM_SYS_WRAP:
		domain_name = "PM_SYS_WRAP";
		break;
	case PM_DMC:
		domain_name = "PM_DMC";
		break;
	case PM_ETH:
		domain_name = "PM_ETH";
		break;
	case PM_AUDIO_PDM:
		domain_name = "PM_AUDIO_PDM";
		break;

	default:
		printf("Error: %s wrong\n", __func__);
		break;
	}

	return domain_name;
}

void start_hw_pwrctrl_cpu_on(int id)
{
	switch (id) {
	case 0:
		*P_PWRCTRL_CPU0_FSM_START_ON = 1;
		break;
	case 1:
		*P_PWRCTRL_CPU1_FSM_START_ON = 1;
		break;
	case 2:
		*P_PWRCTRL_CPU2_FSM_START_ON = 1;
		break;
	case 3:
		*P_PWRCTRL_CPU3_FSM_START_ON = 1;
		break;
	case 0xff:
		*P_PWRCTRL_CPUTOP_FSM_START_ON = 1;
		break;
	}
}

void do_reset(enum PM domain, uint32_t pwr_state)
{
	int id = get_domain_id(domain);

	if (pwr_state == PWR_OFF) {
		switch (domain) {
		case PM_CPU_PWR:
			*P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 |= (1 << 0);
			break;
		case PM_CPU_CORE0:
			*P_PWRCTRL_CPU0_AUTO_OFF_CTRL0 |= (1 << 0);
			break;
		case PM_CPU_CORE1:
			*P_PWRCTRL_CPU1_AUTO_OFF_CTRL0 |= (1 << 0);
			break;
		case PM_CPU_CORE2:
			*P_PWRCTRL_CPU2_AUTO_OFF_CTRL0 |= (1 << 0);
			break;
		case PM_CPU_CORE3:
			*P_PWRCTRL_CPU3_AUTO_OFF_CTRL0 |= (1 << 0);
			break;
		case PM_DSPA:
			*P_PWRCTRL_DSPA_AUTO_OFF_CTRL0 |= (1 << 0);
			break;
		case PM_AOCPU:
			*P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0 |= (1 << 0);
			break;
		default:
			if (id < 32)
				*P_PWRCTRL_FOCRST0 |= (1 << id);
			else
				*P_PWRCTRL_FOCRST1 |= (1 << (id - 32));
			break;
		}
	} else {
		switch (domain) {
		case PM_CPU_PWR:
			*P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 &= ~(1 << 0);
			break;
		case PM_CPU_CORE0:
			*P_PWRCTRL_CPU0_AUTO_OFF_CTRL0 &= ~(1 << 0);
			break;
		case PM_CPU_CORE1:
			*P_PWRCTRL_CPU1_AUTO_OFF_CTRL0 &= ~(1 << 0);
			break;
		case PM_CPU_CORE2:
			*P_PWRCTRL_CPU2_AUTO_OFF_CTRL0 &= ~(1 << 0);
			break;
		case PM_CPU_CORE3:
			*P_PWRCTRL_CPU3_AUTO_OFF_CTRL0 &= ~(1 << 0);
			break;
		case PM_DSPA:
			*P_PWRCTRL_DSPA_AUTO_OFF_CTRL0 &= ~(1 << 0);
			break;
		case PM_AOCPU:
			*P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0 &= ~(1 << 0);
			break;
		default:
			if (id < 32)
				*P_PWRCTRL_FOCRST0 &= ~(1 << id);
			else
				*P_PWRCTRL_FOCRST1 &= ~(1 << (id - 32));
			break;
		}
	}
}

void power_switch_to_domains(enum PM domain, uint32_t pwr_state)
{
	if (pwr_state == PWR_ON) {
		printf("[PWR]: Power on %s domain.\n", get_domain_name(domain));
		// assert reset
		do_reset(domain, PWR_OFF);

		// Powerup Power Domain
		do_power_switch(domain, PWR_ON);

		udelay(100);

		// Powerup memories
		do_power_memory(domain, PWR_ON);

		udelay(100);

		if (domain >= FSM_NUM) {
			///////////////// for slave types //////////////
			// deassert reset_n before removing iso
			////////////////////////////////////////////////
			// deassert reset
			do_reset(domain, PWR_ON);

			// remove isolation
			do_iso_en(domain, PWR_ON);
		} else {
			///////////////// for master types //////////////
			// remove iso before deasserting reset_n
			////////////////////////////////////////////////
			// remove isolation
			do_iso_en(domain, PWR_ON);

			udelay(5);

			// deassert reset
			do_reset(domain, PWR_ON);
		}

	} else {
		printf("[PWR]: power off %s domain.\n", get_domain_name(domain));
		// reset
		do_reset(domain, PWR_OFF);

		// add isolation to domain
		do_iso_en(domain, PWR_OFF);

		// Power down memories
		do_power_memory(domain, PWR_OFF);
		// Power off  domain
		do_power_switch(domain, PWR_OFF);
	}
}

void start_hw_pwrctrl_fsm_off(enum PM domain)
{
	switch (domain) {
	case PM_CPU_PWR:
		*P_PWRCTRL_CPUTOP_FSM_START_OFF = 1;
		break;
	case PM_CPU_CORE0:
		*P_PWRCTRL_CPU0_FSM_START_OFF = 1;
		break;
	case PM_CPU_CORE1:
		*P_PWRCTRL_CPU1_FSM_START_OFF = 1;
		break;
	case PM_CPU_CORE2:
		*P_PWRCTRL_CPU2_FSM_START_OFF = 1;
		break;
	case PM_CPU_CORE3:
		*P_PWRCTRL_CPU3_FSM_START_OFF = 1;
		break;
	case PM_DSPA:
		*P_PWRCTRL_DSPA_FSM_START_OFF = 1;
		break;
	case PM_AOCPU:
		*P_PWRCTRL_AOCPU_FSM_START_OFF = 1;
		break;
	default:
		printf("Error: %s wrong!\n", __func__);
		break;
	}
}

uint32_t addr;
void power_switch_to_wraper(uint32_t pwr_state)
{
	if (pwr_state == PWR_OFF) {
		addr = REG32(CPUCTRL_SYS_CPU_CFG2 + ((0 & 0xff) << 2));
		udelay(500);
		dump_fsm_status();
		power_switch_to_domains(PM_SYS_WRAP, PWR_OFF);
		udelay(2000);
	} else {
		power_switch_to_domains(PM_SYS_WRAP, PWR_ON);
		udelay(500);
		start_hw_pwrctrl_cpu_on(PM_CPU_PWR);
		start_hw_pwrctrl_cpu_on(0x0);
	}
}

void dump_fsm_status(void)
{
	//BIT4=1 core0 enter wfi,bit5 core1 ...
	printf("sys_cpu_status7=0x%x\n", *P_CPUCTRL_SYS_CPU_STATUS7);
	printf("fsm_sts0=0x%x\n", *P_PWRCTRL_CPU0_FSM_STS0);
	printf("fsm_sts1=0x%x\n", *P_PWRCTRL_CPU1_FSM_STS0);
	printf("fsm_sts2=0x%x\n", *P_PWRCTRL_CPU2_FSM_STS0);
	printf("fsm_sts3=0x%x\n", *P_PWRCTRL_CPU3_FSM_STS0);
	printf("fsm_sts_top=0x%x\n", *P_PWRCTRL_CPUTOP_FSM_STS0);
}
