AOCPU Wakeup Flow {#aocpu_wakeup_flow}
===============

## Wakeup Source ##

PATH: [../bl30/rtos_sdk/drivers_aocpu/str/suspend.h]

Supported wakeup source:

	/* wake up reason */
	#define UDEFINED_WAKEUP		0	//"undefine"
	#define CHARGING_WAKEUP		1	//"charging"
	#define REMOTE_WAKEUP		2	//"remote"(ir)
	#define RTC_WAKEUP			3	//"rtc"
	#define BT_WAKEUP			4	//"bt"
	#define WIFI_WAKEUP			5	//"wifi"(WOW)
	#define POWER_KEY_WAKEUP	6	//"powerkey"
	#define AUTO_WAKEUP			7	//"auto"
	#define CEC_WAKEUP			8	//"cec"
	#define REMOTE_CUS_WAKEUP	9	//"remote_cus"
	#define ETH_PMT_WAKEUP		10	//"eth"(WOL)
	#define CECB_WAKEUP			11	//"cecb"
	#define ETH_PHY_GPIO		12	//"eth_gpio"
	#define VAD_WAKEUP			13	//"vad"
	#define HDMI_RX_WAKEUP		14	//"hdmirx_plugin"
	#define UART_RX_WAKEUP		15

## Wakeup Flow ##

### Prepare ###

#### 1. create str task in main.c ####

	- hw_business_process();
	--- create_str_task();

PATH: [../bl30/rtos_sdk/drivers_aocpu/str/suspend.c]

	xTaskCreate(vSTRTask, "STR_task", configMINIMAL_STACK_SIZE, NULL, 3, NULL);

#### 2. vSTRTask ####

PATH: [../bl30/rtos_sdk/drivers_aocpu/str/suspend.c]

create xSTRQueue:

	xSTRQueue = xQueueCreate(STR_QUEUE_LENGTH, STR_QUEUE_ITEM_SIZE);

create xSTRSemaphore :

	xSTRSemaphore = xSemaphoreCreateBinary();

#### 3. Wait for wakeup irq to set xSTRSemaphore ####

Task blocked, waiting for xSTRSemaphore:

	xSemaphoreTake(xSTRSemaphore, portMAX_DELAY);

### Suspend ###

#### 1. Arm enter suspend flow, it would send suspend signal of mailbox. ####

#### 2. Setting “xSTRSemaphore” in AOCPU mailbox callback ####

	- xMboxSuspend_Sem;
	--- STR_Start_Sem_Give();
	----- xSemaphoreGive(xSTRSemaphore);

#### 3. vSTRTask get xSTRSemaphore and enter suspend flow ####

PATH: [../bl30/rtos_sdk/boards/riscv/${BOARD}/power.c]

	void system_suspend(uint32_t pm);

init/enable wakeup irq source:

	str_hw_init();

Turn off the power and adjust the voltage:

	str_power_off(shutdown_flag);

// common flow, some boards related would be described in [../bl30/rtos_sdk/boards/riscv/${BOARD}/power.c]
* power off vcc_5v
* power off vcc_3v3
* turn down vddee
* power off vddcpu

switch clk (for partial chips)

	vCLK_suspend(shutdown_flag);

### Resume ###

#### 1. STRTask waits for message of xSTRQueue ####

	while (xQueueReceive(xSTRQueue, &buffer, portMAX_DELAY))

#### 2. One of the enabled wakeup source irq asserted ####

send message of xSTRQueue in the irq’s callback:

	STR_Wakeup_src_Queue_Send_FromISR

#### 3. vSTRTask gets the message of xSTRQueue, then enter resume flow ####

	void system_resume(uint32_t pm);

switch clk (for partial chips):

	vCLK_resume(shutdown_flag);

Turn on the power and adjust the voltage:

	str_power_on(shutdown_flag);

// common flow, some boards related would be described in [../bl30/rtos_sdk/boards/riscv/${BOARD}/power.c]
* power on vddcpu
* turn up vddee
* power on vcc_3v3
* power on vcc_5v

disable wakeup irq source:

	str_hw_disable();

wakeup ap:

PATH: [../bl30/rtos_sdk/soc/riscv/${SOC}/wakeup.h]

	wakeup_ap();

---
### AOCPU Wakeup Flow Chart ###

![](@ref images/aocpu_wakeup_flow.png)
