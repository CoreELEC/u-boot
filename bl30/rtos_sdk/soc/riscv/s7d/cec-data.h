/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __CEC_S7D_DATA_H__
#define __CEC_S7D_DATA_H__

/*s7D data define*/

/*GPIOH_3 FUNC4:ceca, FUNC1:ceca FUNC2:cecb*/
#define CEC_PIN_MX GPIOH_3	//TODO
#define CEC_PIN_FUNC PIN_FUNC2
#define CEC_IP 1 /* 0: cec a, 1: cecb*/

//reg table define
//#define CEC_CHIP_SEL_T7
//#define CEC_CHIP_SEL_T5
//#define CEC_CHIP_SEL_SC2
#define CEC_CHIP_SEL_S7D

#define CEC_ON 1

/*for compile pass, not defined in register.h, wait cec owner fix*/
#define CLKCTRL_CECA_CTRL0 CLKCTRL_CECB_CTRL0
#define CLKCTRL_CECA_CTRL1 CLKCTRL_CECB_CTRL1
#define CECA_GEN_CNTL CECB_GEN_CNTL
#define CECA_RW_REG CECB_RW_REG
#define CECA_INTR_MASKN CECB_INTR_MASKN
#define CECA_INTR_CLR CECB_INTR_CLR
#define CECA_INTR_STAT CECB_INTR_STAT
#endif
