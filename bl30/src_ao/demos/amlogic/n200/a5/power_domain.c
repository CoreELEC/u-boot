/*
 * Copyright (C) 2014-2021 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FreeRTOS.h"
#include <common.h>
#include "util.h"
#include "power_domain.h"
#include "secure_apb.h"

int get_domain_id(PM_E domain)
{
	int id = 0;

	id = (int)domain;

	return(id);
}

void do_power_switch(PM_E domain, uint32_t pwr_state)
{
	int id = get_domain_id(domain);

	if (pwr_state == PWR_OFF) {
		switch (domain) {
			case PM_CPU_PWR:
				*P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 |= (1<<8);
				break;
			case PM_CPU_CORE0:  *P_PWRCTRL_CPU0_AUTO_OFF_CTRL0   |= (1<<8);  break;
			case PM_CPU_CORE1:  *P_PWRCTRL_CPU1_AUTO_OFF_CTRL0   |= (1<<8);  break;
			case PM_CPU_CORE2:  *P_PWRCTRL_CPU2_AUTO_OFF_CTRL0   |= (1<<8);  break;
			case PM_CPU_CORE3:  *P_PWRCTRL_CPU3_AUTO_OFF_CTRL0   |= (1<<8);  break;
			case PM_DSPA:       *P_PWRCTRL_DSPA_AUTO_OFF_CTRL0   |= (1<<8);  break;
			case PM_AOCPU:      *P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0  |= (1<<8);  break;
			default:
				if (id < 32) {
					*P_PWRCTRL_PWR_OFF0 |=  (1<<id);
				} else {
					*P_PWRCTRL_PWR_OFF1 |=  (1<<(id-32));
				}
				break;
		}
	} else {
		switch (domain) {
			case PM_CPU_PWR:    *P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 &= ~(1<<8);  break;
			case PM_CPU_CORE0:  *P_PWRCTRL_CPU0_AUTO_OFF_CTRL0   &= ~(1<<8);  break;
			case PM_CPU_CORE1:  *P_PWRCTRL_CPU1_AUTO_OFF_CTRL0   &= ~(1<<8);  break;
			case PM_CPU_CORE2:  *P_PWRCTRL_CPU2_AUTO_OFF_CTRL0   &= ~(1<<8);  break;
			case PM_CPU_CORE3:  *P_PWRCTRL_CPU3_AUTO_OFF_CTRL0   &= ~(1<<8);  break;
			case PM_DSPA:       *P_PWRCTRL_DSPA_AUTO_OFF_CTRL0   &= ~(1<<8);  break;
			case PM_AOCPU:      *P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0  &= ~(1<<8);  break;
			default:
				if (id < 32) {
					*P_PWRCTRL_PWR_OFF0 &= ~(1<<id);
				} else {
					*P_PWRCTRL_PWR_OFF1 &= ~(1<<(id-32));
				}
				break;
		}
	}
}

void do_power_memory(PM_E domain, uint32_t pwr_state)
{
	if (pwr_state == PWR_OFF) {
		switch (domain) {
			case PM_CPU_PWR:    *P_PWRCTRL_CPUTOP_MEMPD_INIT_SET   = 0xfffff;   break;
			case PM_CPU_CORE0:  *P_PWRCTRL_CPU0_MEMPD_INIT_SET     = 0x3ffffff; break;
			case PM_CPU_CORE1:  *P_PWRCTRL_CPU1_MEMPD_INIT_SET     = 0x3ffffff; break;
			case PM_CPU_CORE2:  *P_PWRCTRL_CPU2_MEMPD_INIT_SET     = 0x3ffffff; break;
			case PM_CPU_CORE3:  *P_PWRCTRL_CPU3_MEMPD_INIT_SET     = 0x3ffffff; break;
			case PM_DSPA:       *P_PWRCTRL_DSPA_MEMPD_INIT_SET     = 0xffff;    break;
			case PM_AOCPU:      break;
			case PM_NNA:        *P_PWRCTRL_MEM_PD0 = 0xffffffff;
								*P_PWRCTRL_MEM_PD1 = 0xffffffff;     break;
			case PM_AUDIO:      *P_PWRCTRL_MEM_PD4   |= 0xffff3cff;  break;
			case PM_SDIOA:      *P_PWRCTRL_MEM_PD5   |= (0x3<<8);    break;
			case PM_EMMC:       *P_PWRCTRL_MEM_PD5   |= (0x3<<10);   break;
			case PM_USB_COMB:   *P_PWRCTRL_MEM_PD11  |= (0X3<<10);   break;
			case PM_SYS_WRAP:   *P_PWRCTRL_MEM_PD4   |= (0x3<<8 | 0x3<<14);    break;  // ROM & KL
			case PM_ETH:        *P_PWRCTRL_MEM_PD11  |= (0x3<<8);    break;
			//case PM_DDR:        *P_PWRCTRL_MEM_PD11  |= (0x3<<12);   break;
			case PM_RSA:        *P_PWRCTRL_MEM_PD5   |= (0x3<<16);   break;
			case PM_AUDIO_PDM:  *P_PWRCTRL_MEM_PD5   |= (0x3<<4);    break;
			case PM_DMC:        *P_PWRCTRL_MEM_PD5   |= (0x3<<6);    break;
			default:            break;
		}
	} else {
		switch (domain) {
			case PM_CPU_PWR:    *P_PWRCTRL_CPUTOP_MEMPD_INIT_SET   = 0x0;  break;
			case PM_CPU_CORE0:  *P_PWRCTRL_CPU0_MEMPD_INIT_SET     = 0x0;  break;
			case PM_CPU_CORE1:  *P_PWRCTRL_CPU1_MEMPD_INIT_SET     = 0x0;  break;
			case PM_CPU_CORE2:  *P_PWRCTRL_CPU2_MEMPD_INIT_SET     = 0x0;  break;
			case PM_CPU_CORE3:  *P_PWRCTRL_CPU3_MEMPD_INIT_SET     = 0x0;  break;
			case PM_DSPA:       *P_PWRCTRL_DSPA_MEMPD_INIT_SET     = 0x0;  break;
			case PM_AOCPU:      break;
			case PM_NNA:        *P_PWRCTRL_MEM_PD0 = 0x0;
								*P_PWRCTRL_MEM_PD1 = 0x0;               break;
			case PM_AUDIO:      *P_PWRCTRL_MEM_PD4   &= ~0xffff3cff;    break;
			case PM_SDIOA:      *P_PWRCTRL_MEM_PD5   &= ~(0x3<<8);      break;
			case PM_EMMC:       *P_PWRCTRL_MEM_PD5   &= ~(0x3<<10);     break;
			case PM_USB_COMB:   *P_PWRCTRL_MEM_PD11  &= ~(0X3<<10);     break;
			case PM_SYS_WRAP:   *P_PWRCTRL_MEM_PD4   &= ~(0x3<<8 | 0x3<<14);      break;  // ROM & KL
			case PM_ETH:        *P_PWRCTRL_MEM_PD11  &= ~(0X3<<8);      break;
			//case PM_DDR:        *P_PWRCTRL_MEM_PD11  &= ~(0x3<<12);     break;
			case PM_RSA:        *P_PWRCTRL_MEM_PD5   &= ~(0X3<<16);     break;
			case PM_AUDIO_PDM:  *P_PWRCTRL_MEM_PD5   &= ~(0X3<<4);      break;
			case PM_DMC:        *P_PWRCTRL_MEM_PD5   &= ~(0x3<<6);      break;
			default:            break;
		}
	}
}

void do_iso_en(PM_E domain, uint32_t pwr_state)
{
	int id = get_domain_id(domain);

	if (pwr_state == PWR_OFF) {
		switch (domain) {
			case PM_CPU_PWR:    *P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 |= (1<<4);  break;
			case PM_CPU_CORE0:  *P_PWRCTRL_CPU0_AUTO_OFF_CTRL0   |= (1<<4);  break;
			case PM_CPU_CORE1:  *P_PWRCTRL_CPU1_AUTO_OFF_CTRL0   |= (1<<4);  break;
			case PM_CPU_CORE2:  *P_PWRCTRL_CPU2_AUTO_OFF_CTRL0   |= (1<<4);  break;
			case PM_CPU_CORE3:  *P_PWRCTRL_CPU3_AUTO_OFF_CTRL0   |= (1<<4);  break;
			case PM_DSPA:       *P_PWRCTRL_DSPA_AUTO_OFF_CTRL0   |= (1<<4);  break;
			//case PM_DSPB:       *P_PWRCTRL_DSPB_AUTO_OFF_CTRL0   |= (1<<4);  break;
			case PM_AOCPU:      *P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0  |= (1<<4);  break;
			default:
				if (id < 32) {
					*P_PWRCTRL_ISO_EN0 |=  (1<<id);
				} else {
					*P_PWRCTRL_ISO_EN1 |=  (1<<(id-32));
				}
				break;
		}
	} else {
		switch (domain) {
			case PM_CPU_PWR:    *P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 &= ~(1<<4);  break;
			case PM_CPU_CORE0:  *P_PWRCTRL_CPU0_AUTO_OFF_CTRL0   &= ~(1<<4);  break;
	1		case PM_CPU_CORE1:  *P_PWRCTRL_CPU1_AUTO_OFF_CTRL0   &= ~(1<<4);  break;
			case PM_CPU_CORE2:  *P_PWRCTRL_CPU2_AUTO_OFF_CTRL0   &= ~(1<<4);  break;
			case PM_CPU_CORE3:  *P_PWRCTRL_CPU3_AUTO_OFF_CTRL0   &= ~(1<<4);  break;
			case PM_DSPA:       *P_PWRCTRL_DSPA_AUTO_OFF_CTRL0   &= ~(1<<4);  break;
			//case PM_DSPB:       *P_PWRCTRL_DSPB_AUTO_OFF_CTRL0   &= ~(1<<4);  break;
			case PM_AOCPU:      *P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0  &= ~(1<<4);  break;
			default:
				if (id < 32) {
					*P_PWRCTRL_ISO_EN0 &= ~(1<<id);
				} else {
					*P_PWRCTRL_ISO_EN1 &= ~(1<<(id-32));
				}
				break;
		}
	}
}

void do_reset(PM_E domain, uint32_t pwr_state)
{
	int id = get_domain_id(domain);

	if (pwr_state == PWR_OFF) {
		switch (domain) {
			case PM_CPU_PWR:
				*P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 |= (1<<0);
				break;
			case PM_CPU_CORE0:  *P_PWRCTRL_CPU0_AUTO_OFF_CTRL0   |= (1<<0);  break;
			case PM_CPU_CORE1:  *P_PWRCTRL_CPU1_AUTO_OFF_CTRL0   |= (1<<0);  break;
			case PM_CPU_CORE2:  *P_PWRCTRL_CPU2_AUTO_OFF_CTRL0   |= (1<<0);  break;
			case PM_CPU_CORE3:  *P_PWRCTRL_CPU3_AUTO_OFF_CTRL0   |= (1<<0);  break;
			case PM_DSPA:       *P_PWRCTRL_DSPA_AUTO_OFF_CTRL0   |= (1<<0);  break;
			//case PM_DSPB:       *P_PWRCTRL_DSPB_AUTO_OFF_CTRL0   |= (1<<0);  break;
			case PM_AOCPU:      *P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0  |= (1<<0);  break;
			default:
				if (id < 32) {
					*P_PWRCTRL_FOCRST0 |=  (1<<id);
				} else {
					*P_PWRCTRL_FOCRST1 |=  (1<<(id-32));
				}
				break;
		}
	} else {
		switch (domain) {
			case PM_CPU_PWR:    *P_PWRCTRL_CPUTOP_AUTO_OFF_CTRL0 &= ~(1<<0);  break;
			case PM_CPU_CORE0:  *P_PWRCTRL_CPU0_AUTO_OFF_CTRL0   &= ~(1<<0);  break;
			case PM_CPU_CORE1:  *P_PWRCTRL_CPU1_AUTO_OFF_CTRL0   &= ~(1<<0);  break;
			case PM_CPU_CORE2:  *P_PWRCTRL_CPU2_AUTO_OFF_CTRL0   &= ~(1<<0);  break;
			case PM_CPU_CORE3:  *P_PWRCTRL_CPU3_AUTO_OFF_CTRL0   &= ~(1<<0);  break;
			case PM_DSPA:       *P_PWRCTRL_DSPA_AUTO_OFF_CTRL0   &= ~(1<<0);  break;
			//case PM_DSPB:       *P_PWRCTRL_DSPB_AUTO_OFF_CTRL0   &= ~(1<<0);  break;
			case PM_AOCPU:      *P_PWRCTRL_AOCPU_AUTO_OFF_CTRL0  &= ~(1<<0);  break;
			default:
				if (id < 32) {
					*P_PWRCTRL_FOCRST0 &= ~(1<<id);
				} else {
					*P_PWRCTRL_FOCRST1 &= ~(1<<(id-32));
				}
				break;
		}
	}
}

void power_switch_to_domains(PM_E domain, uint32_t pwr_state)
{
	if (pwr_state == PWR_ON) {
		st_printf("[PWR]: Power on %s domain.\n", get_domain_name(domain));

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

			// remove isolations
			do_iso_en(domain, PWR_ON);
		} else {
		///////////////// for master types //////////////
		// remove iso before deasserting reset_n
		////////////////////////////////////////////////
		// remove isolations
		do_iso_en(domain, PWR_ON);

		udelay(5);

		// deassert reset
		do_reset(domain, PWR_ON);
		}

	} else {
		st_printf("[PWR]: power off %s domain.\n", get_domain_name(domain));

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

void start_hw_pwrctrl_fsm_off(PM_E domain)
{
	//int idx = get_fsm_idx(domain);

	//smp_printf("pwrctrl_fsm: Power off domain %s\n", get_domain_name(domain));
	//if(domain == PM_AOCPU) {
	//    stimulus_print("pwrctrl_fsm: Power off domain: PM_AOCPU\n");
	//} else if(domain == PM_DSPA) {
	//    stimulus_print("pwrctrl_fsm: Power off domain: PM_DSPA\n");
	//} else {
	//    stimulus_print("pwrctrl_fsm: Power off domain\n");
	//}
	//*(hw_pwrctrl[idx].fsm_start_off) = 1;

	switch (domain) {
		case PM_CPU_PWR:        *P_PWRCTRL_CPUTOP_FSM_START_OFF = 1;  break;
		case PM_CPU_CORE0:      *P_PWRCTRL_CPU0_FSM_START_OFF = 1;    break;
		case PM_CPU_CORE1:      *P_PWRCTRL_CPU1_FSM_START_OFF = 1;    break;
		case PM_CPU_CORE2:      *P_PWRCTRL_CPU2_FSM_START_OFF = 1;    break;
		case PM_CPU_CORE3:      *P_PWRCTRL_CPU3_FSM_START_OFF = 1;    break;
		case PM_DSPA:           *P_PWRCTRL_DSPA_FSM_START_OFF = 1;    break;
		case PM_AOCPU:          *P_PWRCTRL_AOCPU_FSM_START_OFF = 1;   break;
		default:  stimulus_print("Error: start_hw_pwrctrl_fsm_off wrong!\n");  stimulus_finish_fail(22);  break;
	}
}
