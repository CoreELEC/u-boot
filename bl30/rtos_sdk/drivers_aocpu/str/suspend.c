/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "FreeRTOS.h" /* Must come first. */
#include "task.h" /* RTOS task related API prototypes. */
#include "queue.h" /* RTOS queue related API prototypes. */
#include "timers.h" /* Software timer related API prototypes. */
#include "semphr.h" /* Semaphore related API prototypes. */

#include <unistd.h>
#include <string.h>

#include "n200_func.h"
#include "common.h"
#include "riscv_encoding.h"
#include "gcc_compiler_attributes.h"
#include "suspend.h"
#include "power.h"

#include "hdmi_cec.h"
#include "vrtc.h"
#include "rtc.h"
#include "mailbox-api.h"
#include "eth.h"
#include "wakeup.h"
#include "stick_mem.h"
#include "pm.h"
#include "suspend_debug.h"

SemaphoreHandle_t xSTRSemaphore;
QueueHandle_t xSTRQueue;
SemaphoreHandle_t xSTRFlagSem;
uint32_t suspend_flag;
#if BL30_SUSPEND_DEBUG_EN
uint32_t suspend_debug_flag;
static TaskHandle_t printTask;
#endif
uint32_t power_mode;

struct WakeUp_Reason vWakeupReason[] = {
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
	[ETH_PHY_GPIO] = { .name = "eth_gpio" },
	[VAD_WAKEUP] = { .name = "vad" },
	[HDMI_RX_WAKEUP] = { .name = "hdmirx_plugin" },
};

void set_suspend_flag(void)
{
	taskENTER_CRITICAL();
	suspend_flag = 1;
	vETHEnableIrq();
	taskEXIT_CRITICAL();
}

__weak void vDDR_suspend(uint32_t st_f)
{
	(void)st_f;
}

__weak void vDDR_resume(uint32_t st_f)
{
	(void)st_f;
}

__weak void alarm_set(void)
{
}

__weak void alarm_clr(void)
{
}

__weak void vRTC_update(void)
{
}

__weak void vCreat_alarm_timer(void)
{
}

__weak void store_rtc(void)
{
}

__weak void vCLK_suspend(uint32_t st_f)
{
	(void)st_f;
}

__weak void vCLK_resume(uint32_t st_f)
{
	(void)st_f;
}

#if BL30_SUSPEND_DEBUG_EN
inline void split_suspend_flag(uint32_t *temp)
{
	suspend_debug_flag = (*temp) & SUSPEND_DEBUG_MASK;
	*temp = (*temp) & ~SUSPEND_DEBUG_MASK;
}

inline uint32_t get_suspend_flag(void)
{
	return suspend_debug_flag;
}

void vDebugPrintTask(void *pvParameters)
{
	/*make compiler happy*/
	(void) pvParameters;

	for ( ;; ) {
		printf("vPTask1...\n");
		vTaskDelay(pdMS_TO_TICKS(TEST_TASK1_DELAY));
	}
}
#endif

void system_resume(uint32_t pm)
{
	uint32_t shutdown_flag = 0;

#if BL30_SUSPEND_DEBUG_EN
	enter_func_print();
#endif

	if (pm == 0xf)
		shutdown_flag = 1;
	vCLK_resume(shutdown_flag);
	/*Need clr alarm ASAP*/
	alarm_clr();
	str_power_on(shutdown_flag);
#if BL30_SUSPEND_DEBUG_EN
	if (!IS_EN(BL30_SKIP_DDR_SUSPEND))
#endif
		vDDR_resume(shutdown_flag);
	str_hw_disable();
	vRTC_update();
	wakeup_ap();
	clear_wakeup_trigger();
	/*Shutdown*/
	if (shutdown_flag) {
		/* Store RTC time for a5 temporarily*/
		store_rtc();
		watchdog_reset_system();
	}

#if BL30_SUSPEND_DEBUG_EN
	stop_debug_task();
	exit_func_print();
#endif
}

void system_suspend(uint32_t pm)
{
	uint32_t shutdown_flag = 0;
#if BL30_SUSPEND_DEBUG_EN
	split_suspend_flag(&pm);
	enter_func_print();
	start_debug_task();
#endif
	if (pm == 0xf)
		shutdown_flag = 1;

	/*Need set alarm ASAP*/
	alarm_set();
	str_hw_init();
	/*Set flag befor delay. It can be wakeup during delay*/
	set_suspend_flag();
	/*Delay 500ms for FSM switch to off*/
	vTaskDelay(pdMS_TO_TICKS(500));
#if BL30_SUSPEND_DEBUG_EN
	if (!IS_EN(BL30_SKIP_DDR_SUSPEND))
#endif
		vDDR_suspend(shutdown_flag);
	str_power_off(shutdown_flag);
	vCLK_suspend(shutdown_flag);
#if BL30_SUSPEND_DEBUG_EN
	exit_func_print();
#endif
}

void set_reason_flag(char exit_reason)
{
	uint32_t val;

	val = REG32(WAKEUP_REASON_STICK_REG) & ~0xff;
	val |= EXIT_REASON_EXTENSION_FLAG;
	val |= (exit_reason & 0x7f);
	REG32(WAKEUP_REASON_STICK_REG) = val;
}

uint32_t get_reason_flag(void)
{
	return REG32(WAKEUP_REASON_STICK_REG) & 0x7f;
}

uint32_t get_stick_reboot_flag(void)
{
#if (configSUPPORT_STICK_MEM == 1)
	return last_stick_reboot_flag;
#else
	printf("Don't support stick memory!\r\n");
	return 0;
#endif
}

void *xMboxGetWakeupReason(void *msg)
{
	*(uint32_t *)msg = get_reason_flag();
	return NULL;
}

void *xMboxClrWakeupReason(void *msg)
{
	(void)msg;
	return NULL;
}

void *xMboxGetStickRebootFlag(void *msg)
{
	*(uint32_t *)msg = get_stick_reboot_flag();

	return NULL;
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
	uint32_t flag = 0;
	UBaseType_t uxSavedInterruptStatus;

	uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
	if (suspend_flag) {
		suspend_flag = 0;
		flag = 1;
	}
	taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);

	if (flag)
		xQueueSendFromISR(xSTRQueue, src, &xHigherPriorityTaskWoken);
}

void STR_Wakeup_src_Queue_Send(uint32_t *src)
{
	uint32_t flag = 0;

	taskENTER_CRITICAL();
	if (suspend_flag) {
		suspend_flag = 0;
		flag = 1;
	}
	taskEXIT_CRITICAL();

	if (flag)
		xQueueSend(xSTRQueue, src, portMAX_DELAY);
}

void *xMboxSuspend_Sem(void *msg)
{
	power_mode = *(uint32_t *)msg;

	printf("power_mode=0x%x\n", power_mode & POWER_MODE_MASK);
	STR_Start_Sem_Give();

	return NULL;
}

#define FREEZE_ENTER 0x01
#define FREEZE_EXIT 0x02

void *xMboxpm_sem(void *msg);
void *xMboxpm_sem(void *msg)
{
	uint32_t mode = *(uint32_t *)msg;

#ifdef CONFIG_PM
	if (mode == FREEZE_ENTER)
		pm_enter();
	else if (mode == FREEZE_EXIT)
		wakeup_ap_from_kernel();
#endif

	return NULL;
}

static void vSTRTask(void *pvParameters)
{
	/*make compiler happy*/
	uint32_t buffer = UDEFINED_WAKEUP;
	uint32_t exit_reason = 0;

	(void)pvParameters;
	xSTRQueue = xQueueCreate(STR_QUEUE_LENGTH, STR_QUEUE_ITEM_SIZE);
	configASSERT(xSTRQueue);
	xSTRSemaphore = xSemaphoreCreateBinary();
	configASSERT(xSTRSemaphore);

	while (1) {
		xSemaphoreTake(xSTRSemaphore, portMAX_DELAY);
		system_suspend(power_mode);
		while (xQueueReceive(xSTRQueue, &buffer, portMAX_DELAY)) {
			switch (buffer) {
			case REMOTE_WAKEUP:
				exit_reason = REMOTE_WAKEUP;
				break;
			case RTC_WAKEUP:
				exit_reason = RTC_WAKEUP;
				break;
			case BT_WAKEUP:
				exit_reason = BT_WAKEUP;
				break;
			case CEC_WAKEUP:
				exit_reason = CEC_WAKEUP;
				break;
			case CECB_WAKEUP:
				exit_reason = CECB_WAKEUP;
				break;
			case REMOTE_CUS_WAKEUP:
				exit_reason = REMOTE_CUS_WAKEUP;
				break;
			case POWER_KEY_WAKEUP:
				exit_reason = POWER_KEY_WAKEUP;
				break;
			case ETH_PMT_WAKEUP:
				exit_reason = ETH_PMT_WAKEUP;
				break;
			case WIFI_WAKEUP:
				exit_reason = WIFI_WAKEUP;
				break;
			case ETH_PHY_GPIO:
				exit_reason = ETH_PHY_GPIO;
				printf("wzh eth gpio\n");
				break;
			case VAD_WAKEUP:
				exit_reason = VAD_WAKEUP;
				break;
			case HDMI_RX_WAKEUP:
				exit_reason = HDMI_RX_WAKEUP;
				break;
			case UART_RX_WAKEUP:
				exit_reason = UART_RX_WAKEUP;
				break;
			default:
				printf("unknown exit_reason %d\n", exit_reason);
				set_suspend_flag();
				break;
			}
			if (exit_reason) {
				printf("exit_reason=%d, %s\n", exit_reason,
				       vWakeupReason[exit_reason].name);
				set_reason_flag((char)exit_reason);
				exit_reason = 0;
				system_resume(power_mode);
				goto loop;
			}
		}
loop:
		continue;
	}
}

void create_str_task(void)
{
	int ret;

	if (xTaskCreate(vSTRTask, "STR_task", configMINIMAL_STACK_SIZE, NULL, 3, NULL) < 0)
		printf("STR_task create fail!!\n");

	vCreat_alarm_timer();

	ret = xInstallRemoteMessageCallbackFeedBack(AOTEE_CHANNEL, MBX_CMD_SUSPEND,
						    xMboxSuspend_Sem, 0);
	if (ret == MBOX_CALL_MAX)
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_SUSPEND);

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_GET_WAKEUP_REASON,
						    xMboxGetWakeupReason, 1);
	if (ret == MBOX_CALL_MAX)
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_GET_WAKEUP_REASON);

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_CLR_WAKEUP_REASON,
						    xMboxClrWakeupReason, 0);

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_GET_STICK_REBOOT_FLAG,
						    xMboxGetStickRebootFlag, 1);
	if (ret == MBOX_CALL_MAX)
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_CLR_WAKEUP_REASON);

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_PM_FREEZE, xMboxpm_sem,
						    1);
	if (ret == MBOX_CALL_MAX)
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_PM_FREEZE);
}
