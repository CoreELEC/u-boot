#include <string.h>
#include "FreeRTOS.h"
#include "ir.h"
#include "keypad.h"
#include "gpio.h"
#include "saradc.h"
#include "suspend.h"

/*KEY ID*/
#define GPIO_KEY_ID_POWER	GPIOD_4
/*WOL GPIO*/
#define WOL_GPIO	GPIOH_6

#define ADC_KEY_ID_MENU		520
#define ADC_KEY_ID_VOL_DEC	521
#define ADC_KEY_ID_VOL_INC	522
#define ADC_KEY_ID_ESC		523
#define ADC_KEY_ID_HOME		524

static void vGpioKeyCallBack(struct xReportEvent event)
{
	uint32_t buf[4] = {0};

	if (event.ulCode == GPIO_KEY_ID_POWER || event.ulCode == get_User_Gpio()) {
		buf[0] = POWER_KEY_WAKEUP;
		STR_Wakeup_src_Queue_Send(buf);
	} else if (event.ulCode == WOL_GPIO) {
		buf[0] = WOL_WAKEUP;
		STR_Wakeup_src_Queue_Send(buf);
	}

	printf("GPIO key event 0x%x, key code %d, responseTicks %d\n",
		event.event, event.ulCode, event.responseTime);
}

static void vAdcKeyCallBack(struct xReportEvent event)
{
	printf("ADC key event 0x%x, key code %d, responseTime %d\n",
		event.event, event.ulCode, event.responseTime);
}

struct xGpioKeyInfo gpioKeyInfo[] = {
	GPIO_KEY_INFO(GPIO_KEY_ID_POWER, HIGH, EVENT_SHORT,
			vGpioKeyCallBack, NULL),
	GPIO_KEY_INFO(WOL_GPIO, HIGH, EVENT_SHORT,
			vGpioKeyCallBack, NULL),
	GPIO_KEY_INFO(GPIO_INVALID, HIGH, EVENT_SHORT,
			vGpioKeyCallBack, NULL)

};

struct xAdcKeyInfo adcKeyInfo[] = {
	ADC_KEY_INFO(ADC_KEY_ID_MENU, 0, SARADC_CH2,
		     EVENT_SHORT,
		     vAdcKeyCallBack, NULL),
	ADC_KEY_INFO(ADC_KEY_ID_VOL_DEC, 574, SARADC_CH2,
		     EVENT_SHORT,
		     vAdcKeyCallBack, NULL),
	ADC_KEY_INFO(ADC_KEY_ID_VOL_INC, 1065, SARADC_CH2,
		     EVENT_SHORT,
		     vAdcKeyCallBack, NULL),
	ADC_KEY_INFO(ADC_KEY_ID_ESC, 1557, SARADC_CH2,
		     EVENT_SHORT | EVENT_LONG,
		     vAdcKeyCallBack, NULL),
	ADC_KEY_INFO(ADC_KEY_ID_HOME, 2048, SARADC_CH2,
		     EVENT_SHORT,
		     vAdcKeyCallBack, NULL)
};

void vKeyPadInit(void)
{
	gpioKeyInfo[2].keyInitInfo.ulKeyId = get_User_Gpio();
	vCreateGpioKey(gpioKeyInfo,
			sizeof(gpioKeyInfo)/sizeof(struct xGpioKeyInfo));
	vGpioKeyEnable();
}

void vKeyPadDeinit(void)
{
	vGpioKeyDisable();
	vDestoryGpioKey();
}
