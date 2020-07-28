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


#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include "myprintf.h"

#include <unistd.h>

#include "n200_eclic.h"
#include "n200_func.h"
#include "common.h"
#include "riscv_encoding.h"
#include "suspend.h"
#include "power.h"

#include "hdmi_cec.h"
#include "vrtc.h"
#include "mailbox-api.h"

void wakeup_ap(void);
void clear_wakeup_trigger(void);
void system_resume(uint32_t pm);
void system_suspend(uint32_t pm);
void set_reason_flag(char exit_reason);
void create_str_task(void);

SemaphoreHandle_t xSTRSemaphore = NULL;
QueueHandle_t xSTRQueue = NULL;

TimerHandle_t xSuspendTimer = NULL;

uint32_t power_mode;

WakeUp_Reason vWakeupReason[] = {
	[UDEFINED_WAKEUP] = { .name = "undefine" },
	[CHARGING_WAKEUP] = { .name = "charging" },
	[REMOTE_WAKEUP] = { .name = "remote" },
	[RTC_WAKEUP] = { .name = "rtc" },
	[BT_WAKEUP] = { .name = "bt" },
	[WIFI_WAKEUP] = { .name = "wifi" },
	[POWER_KEY_WAKEUP] = { .name = "powerkey" },
	[AUTO_WAKEUP] = { .name = "auto" },
	[CEC_WAKEUP] = { .name = "cec" },
	[REMOTE_CUS_WAKEUP] = { .name = "remote_cus" },
	[ETH_PMT_WAKEUP] = { .name = "eth" },
	[CECB_WAKEUP] = { .name = "cecb" },
};

void vCEC_task(void *pvParameters)
{
	u32 ret;
	u32 buf[4] = {0};

	buf[0] = CEC_WAKEUP;

	pvParameters = pvParameters;
	ret = cec_init_config();
	if (!ret)
		goto idle;

	cec_delay(100);
	while (1) {
		//printf("%s 01\n", __func__);
		vTaskDelay(pdMS_TO_TICKS(20));
		cec_suspend_handle();
		if (cec_get_wakup_flag()) {
			STR_Wakeup_src_Queue_Send(buf);
			break;
		}
	}

idle:
	for ( ;; ) {
		vTaskDelay(pdMS_TO_TICKS(2000));
		printf("%s idle\n", __func__);
	}
}

/*use timerB to wakeup AP FSM*/
void wakeup_ap(void)
{
	uint32_t value;
	//uint32_t time_out = 20;

	/*set alarm timer*/
	REG32(SYSCTRL_TIMERB) = 1000;/*1ms*/

	value = REG32(SYSCTRL_TIMERB_CTRL);
	value &= ~((1 << 7) | (0x3) | (1 << 6));
	value |= ((1 << 7) | (0 << 6) | (0x3));
	REG32(SYSCTRL_TIMERB_CTRL) = value;
}

void clear_wakeup_trigger(void)
{
	REG32(SYSCTRL_TIMERB) = 0;
	REG32(SYSCTRL_TIMERB_CTRL) = 0;
}

void system_resume(uint32_t pm)
{
	str_power_on();
	str_hw_disable();
	vRTC_update();
	wakeup_ap();
}

void system_suspend(uint32_t pm)
{
	str_hw_init();
	str_power_off();
}

void set_reason_flag(char exit_reason)
{
	REG32(SYSCTRL_STATUS_REG7) &= ~0xf;
	REG32(SYSCTRL_STATUS_REG7) |= exit_reason;
}

void STR_Start_Sem_Give_FromISR(void)
{
	BaseType_t xHigherPriorityTaskWoken;
	xSemaphoreGiveFromISR(xSTRSemaphore, &xHigherPriorityTaskWoken);
}

void STR_Start_Sem_Give(void)
{
	xSemaphoreGive(xSTRSemaphore);
}


void STR_Wakeup_src_Queue_Send_FromISR(uint32_t *src)
{
	BaseType_t xHigherPriorityTaskWoken;
	xQueueSendFromISR(xSTRQueue, src, &xHigherPriorityTaskWoken);
}

void STR_Wakeup_src_Queue_Send(uint32_t *src)
{
	xQueueSend(xSTRQueue, src, portMAX_DELAY);
}

void xMboxSuspend_Sem(void *msg)
{
	power_mode = *(uint32_t *)msg;

	printf("power_mode=0x%x\n",power_mode);
	STR_Start_Sem_Give();
}

static void vSuspendTest(TimerHandle_t xTimer) {
	uint32_t buf[4] = {0};
	buf[0] = RTC_WAKEUP;
	xTimer = xTimer;
	taskENTER_CRITICAL();
	printf("\r\nvSuspendTest timer ...\r\n");

	STR_Wakeup_src_Queue_Send(buf);
	xSemaphoreGive( xSTRSemaphore );
	taskEXIT_CRITICAL();
}

static void vSTRTask( void *pvParameters )
{
    /*make compiler happy*/
	char buffer[STR_QUEUE_ITEM_SIZE];
	uint32_t exit_reason = 0;

	pvParameters = pvParameters;
    xSTRQueue = xQueueCreate(STR_QUEUE_LENGTH, STR_QUEUE_ITEM_SIZE);
	configASSERT(xSTRQueue);
    xSTRSemaphore = xSemaphoreCreateBinary();
	configASSERT(xSTRSemaphore);

	xSuspendTimer = xTimerCreate("SuspendTest", pdMS_TO_TICKS(5000), pdTRUE, NULL, vSuspendTest);
	//xTimerStart(xSuspendTimer, 0);

	while (1) {
		xSemaphoreTake(xSTRSemaphore, portMAX_DELAY);
		system_suspend(power_mode);
		while (xQueueReceive(xSTRQueue, buffer, portMAX_DELAY))
		{
			switch (buffer[0])
			{
				case REMOTE_WAKEUP:
					exit_reason = REMOTE_WAKEUP;
					break;
				case RTC_WAKEUP:
					exit_reason = RTC_WAKEUP;
					break;
				case CEC_WAKEUP:
					exit_reason = CEC_WAKEUP;
					break;
				case CECB_WAKEUP:
					exit_reason = CECB_WAKEUP;
					break;
				default:
					break;
			}
			if (exit_reason) {
				printf("exit_reason=%d, %s\n",exit_reason, vWakeupReason[exit_reason].name);
				set_reason_flag((char)exit_reason);
				system_resume(power_mode);
				goto loop;
			}
		}
		loop: continue;
	}
}

void create_str_task(void)
{
	int ret;

	if (xTaskCreate( vSTRTask, "STR_task", configMINIMAL_STACK_SIZE, NULL, 3, NULL ) < 0)
		printf("STR_task create fail!!\n");

	ret = xInstallRemoteMessageCallbackFeedBack(AOTEE_CHANNEL, MBX_CMD_SUSPEND,
						xMboxSuspend_Sem, 0);
	if (ret == MBOX_CALL_MAX)
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_SUSPEND);
}

