#ifndef __T5_CEC_DATA_H__
#define __T5_CEC_DATA_H__

#include "secure_apb.h"
#include "gpio.h"

/*T5 data define*/

/*T5 register table enum cec_reg_idx*/
unsigned int cec_reg_tab[] ={
	0xffff,/*AO_CEC_CLK_CNTL_REG0*/
	0xffff,/*AO_CEC_CLK_CNTL_REG1*/
	0xffff,/*AO_CEC_GEN_CNTL*/
	0xffff,/*AO_CEC_RW_REG*/
	0xffff,/*AO_CEC_INTR_MASKN*/
	0xffff,/*AO_CEC_INTR_CLR*/
	0xffff,/*AO_CEC_INTR_STAT*/

	AO_CECB_CLK_CNTL_REG0,
	AO_CECB_CLK_CNTL_REG1,
	AO_CECB_GEN_CNTL,
	AO_CECB_RW_REG,
	AO_CECB_INTR_MASKN,
	AO_CECB_INTR_CLR,
	AO_CECB_INTR_STAT,

	AO_DEBUG_REG0,
	AO_DEBUG_REG1,

	AO_CEC_STICKY_DATA0,
	AO_CEC_STICKY_DATA1,/*port info return val to kernel*/
	AO_CEC_STICKY_DATA2,/*not use*/
	AO_CEC_STICKY_DATA3,/*not use*/
	AO_CEC_STICKY_DATA4,/*not use*/
	AO_CEC_STICKY_DATA5,/*not use*/
	AO_CEC_STICKY_DATA6,/*not use*/
	AO_CEC_STICKY_DATA7,/*not use*/
};

#define CEC_PIN_MX	GPIOW_12
#define CEC_PIN_FUNC	PIN_FUNC1

#endif

