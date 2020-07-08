#ifndef __HDMI_CEC__
#define __HDMI_CEC__


u32 cec_init_config(void);
u32 cec_suspend_handle(void);


/*cec API for suspend*/

/*
 * when power down, create the cec task
 */
void vCEC_task(void *pvParameters);

/*
 * call cec wakup flag, return 1 need wakup
 */
u32 cec_get_wakup_flag(void);

#endif
