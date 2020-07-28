#ifndef __HDMI_CEC__
#define __HDMI_CEC__


u32 cec_init_config(void);
u32 cec_suspend_handle(void);

/*cec API for suspend*/


#define CEC_TASK_PRI			5
#define CEC_TASK_STACK_SIZE		1024
/*
 * when power down, create the cec task
 */
void vCEC_task(void *pvParameters);

/*
 * call cec wakup flag, return 1 need wakup
 */
u32 cec_get_wakup_flag(void);

void cec_req_irq(u32 onoff);
void cec_delay(u32 cnt);
void vCecCallbackInit(void);
#endif
