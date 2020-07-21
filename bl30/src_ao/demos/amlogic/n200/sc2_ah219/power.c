/*
 * Copyright (C) 2014-2018 Amlogic, Inc. All rights reserved.
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
#include "common.h"
#include "gpio.h"
#include "ir.h"
#include "suspend.h"
#include "task.h"

#include "hdmi_cec.h"

static TaskHandle_t cecTask = NULL;

static uint32_t power_key_list[] = {

        0xef10fe01, /* ref tv pwr */
        0xba45bd02, /* small ir pwr */
        0xef10fb04, /* old ref tv pwr */
        0xf20dfe01,
        0xe51afb04
        /* add more */
};

static void vIRHandler(void)
{
	uint32_t buf[4] = {0};
	buf[0] = REMOTE_WAKEUP;
        printf("ir wakeup\n");
        /* do sth below  to wakeup*/
	STR_Wakeup_src_Queue_Send_FromISR(buf);
};


void str_hw_init(void);
void str_hw_disable(void);
void str_power_on(void);
void str_power_off(void);

void str_hw_init(void)
{
	/*enable device & wakeup source interrupt*/
	vIRInit(MODE_HARD_NEC, GPIOD_5, PIN_FUNC1, power_key_list, ARRAY_SIZE(power_key_list), vIRHandler);
	xTaskCreate(vCEC_task, "CECtask", configMINIMAL_STACK_SIZE,
		    NULL, CEC_TASK_PRI, &cecTask);
}


void str_hw_disable(void)
{
	/*disable wakeup source interrupt*/
	vIRDeint();
	if (cecTask) {
		vTaskDelete(cecTask);
		cec_req_irq(0);
	}
}

void str_power_on(void)
{

}

void str_power_off(void)
{

}

