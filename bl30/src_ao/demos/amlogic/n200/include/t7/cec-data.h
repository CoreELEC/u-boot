#ifndef __CEC_SC2_DATA_H__
#define __CEC_SC2_DATA_H__

#include "register.h"
#include "gpio.h"
/*SC2 data define*/


/*T7 same with SC2 register enum cec_reg_idx */
unsigned int cec_reg_tab[] = {
	CLKCTRL_CECA_CTRL0,
	CLKCTRL_CECA_CTRL1,
	CECA_GEN_CNTL,
	CECA_RW_REG,
	CECA_INTR_MASKN,
	CECA_INTR_CLR,
	CECA_INTR_STAT,

	CLKCTRL_CECB_CTRL0,
	CLKCTRL_CECB_CTRL1,
	CECB_GEN_CNTL,
	CECB_RW_REG,
	CECB_INTR_MASKN,
	CECB_INTR_CLR,
	CECB_INTR_STAT,

	SYSCTRL_STATUS_REG0,
	SYSCTRL_STATUS_REG1,

	0xffff,//AO_CEC_STICKY_DATA0,
	0xffff,//AO_CEC_STICKY_DATA1,
	0xffff,//AO_CEC_STICKY_DATA2,
	0xffff,//AO_CEC_STICKY_DATA3,
	0xffff,//AO_CEC_STICKY_DATA4,
	0xffff,//AO_CEC_STICKY_DATA5,
	0xffff,//AO_CEC_STICKY_DATA6,
	0xffff,//AO_CEC_STICKY_DATA7,
};

/*GPIOH_3 FUNC4:ceca, FUNC5:cecb*/
#define CEC_PIN_MX	GPIOW_12
#define CEC_PIN_FUNC	PIN_FUNC1
#define CEC_IP		0	/* 0: cec a, 1: cecb*/

#endif

