/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/* wake up reason*/
#define UDEFINED_WAKEUP 0
#define CHARGING_WAKEUP 1
#define REMOTE_WAKEUP 2
#define RTC_WAKEUP 3
#define BT_WAKEUP 4
#define WIFI_WAKEUP 5
#define POWER_KEY_WAKEUP 6
#define AUTO_WAKEUP 7
#define CEC_WAKEUP 8
#define REMOTE_CUS_WAKEUP 9
#define ETH_PMT_WAKEUP 10
#define CECB_WAKEUP 11
#define ETH_PHY_GPIO 12
#define VAD_WAKEUP 13
#define HDMI_RX_WAKEUP 14

#define STR_QUEUE_LENGTH 32
#define STR_QUEUE_ITEM_SIZE 4
#define EXIT_REASON_EXTENSION_FLAG	(1 << 7)

struct WakeUp_Reason {
	char *name;
};

void vDDR_suspend(uint32_t st_f);
void vDDR_resume(uint32_t st_f);
uint32_t parse_suspend_msg(void *msg);
void vCLK_suspend(uint32_t st_f);
void vCLK_resume(uint32_t st_f);
extern void create_str_task(void);
extern void STR_Start_Sem_Give_FromISR(void);
extern void STR_Start_Sem_Give(void);
extern void STR_Wakeup_src_Queue_Send_FromISR(uint32_t *src);
extern void STR_Wakeup_src_Queue_Send(uint32_t *src);
extern void *xMboxSuspend_Sem(void *msg);
