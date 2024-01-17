/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "fd650.h"
#include "common.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "timer_source.h"
#include <task.h>
#include <fd650_plat.h>
#include <string.h>
#include "mailbox-api.h"
#include "vrtc.h"
#include "timers.h" /* Software timer related API prototypes. */

#define DRIVER_NAME				"FD650"
#define DEFAULT_UDELAY			3

static struct fd650_bus *current_bus;
static uint8_t fd650_state = FD650_STATE_TIME;

static int fd650_sda_get(struct fd650_bus *bus)
{
	int value;

	xGpioSetDir(bus->sda, GPIO_DIR_IN);
	value = xGpioGetValue(bus->sda);
	xGpioSetDir(bus->sda, GPIO_DIR_OUT);

	return value;
}

static void fd650_sda_set(struct fd650_bus *bus, enum GpioOutLevelType bit)
{
	xGpioSetValue(bus->sda, bit);
}

static void fd650_scl_set(struct fd650_bus *bus, enum GpioOutLevelType bit)
{
	xGpioSetValue(bus->scl, bit);
}

void fd650_start(struct fd650_bus *bus)
{
	fd650_sda_set(bus, GPIO_LEVEL_HIGH);
	fd650_scl_set(bus, GPIO_LEVEL_HIGH);
	udelay(bus->udelay);
	fd650_sda_set(bus, GPIO_LEVEL_LOW);
	udelay(bus->udelay);
	fd650_scl_set(bus, GPIO_LEVEL_LOW);
}

void fd650_stop(struct fd650_bus *bus)
{
	fd650_sda_set(bus, GPIO_LEVEL_LOW);
	udelay(bus->udelay);
	fd650_scl_set(bus, GPIO_LEVEL_HIGH);
	udelay(bus->udelay);
	fd650_sda_set(bus, GPIO_LEVEL_HIGH);
	udelay(bus->udelay);
}

void fd650_wrbyte(struct fd650_bus *bus, uint8_t dat)
{
	uint8_t i;

	for (i = 0; i != 8; i++) {
		if (dat & 0x80)
			fd650_sda_set(bus, GPIO_LEVEL_HIGH);
		else
			fd650_sda_set(bus, GPIO_LEVEL_LOW);
		udelay(bus->udelay);
		fd650_scl_set(bus, GPIO_LEVEL_HIGH);
		dat <<= 1;
		udelay(bus->udelay);
		fd650_scl_set(bus, GPIO_LEVEL_LOW);
	}
	fd650_sda_set(bus, GPIO_LEVEL_HIGH);
	udelay(bus->udelay);
	fd650_scl_set(bus, GPIO_LEVEL_HIGH);
	udelay(bus->udelay);
	fd650_scl_set(bus, GPIO_LEVEL_LOW);
}

uint8_t fd650_rdbyte(struct fd650_bus *bus)
{
	uint8_t dat, i;

	fd650_sda_set(bus, GPIO_LEVEL_HIGH);
	dat = 0;
	for (i = 0; i != 8; i++) {
		udelay(bus->udelay);
		fd650_scl_set(bus, GPIO_LEVEL_HIGH);
		udelay(bus->udelay);
		dat <<= 1;
		if (fd650_sda_get(bus))
			dat++;
		fd650_scl_set(bus, GPIO_LEVEL_LOW);
	}
	fd650_sda_set(bus, GPIO_LEVEL_HIGH);
	udelay(bus->udelay);
	fd650_scl_set(bus, GPIO_LEVEL_HIGH);
	udelay(bus->udelay);
	fd650_scl_set(bus, GPIO_LEVEL_LOW);

	return dat;
}

void fd650_write(struct fd650_bus *bus, uint32_t cmd)
{
	fd650_start(bus);
	fd650_wrbyte(bus, ((uint8_t)(cmd >> 7) & 0x3E) | 0x40);
	fd650_wrbyte(bus, (uint8_t)cmd);
	fd650_stop(bus);

}

uint8_t fd650_read(struct fd650_bus *bus)
{
	uint8_t keycode = 0;

	fd650_start(bus);
	fd650_wrbyte(bus, ((uint8_t)(FD650_GET_KEY >> 7) & 0x3E) | (0x01 | 0x40));
	keycode = fd650_rdbyte(bus);
	fd650_stop(bus);
	if ((keycode & 0x00000040) == 0)
		keycode = 0;
	return keycode;
}

#define LEDMAPNUM 22

struct led_bitmap {
	uint8_t character;
	uint8_t bitmap;
};

const struct led_bitmap bcd_decode_tab[LEDMAPNUM] = {
	{'0', 0x3F}, {'1', 0x06}, {'2', 0x5B}, {'3', 0x4F},
	{'4', 0x66}, {'5', 0x6D}, {'6', 0x7D}, {'7', 0x07},
	{'8', 0x7F}, {'9', 0x6F}, {'a', 0x77}, {'A', 0x77},
	{'b', 0x7C}, {'B', 0x7C}, {'c', 0x58}, {'C', 0x39},
	{'d', 0x5E}, {'D', 0x5E}, {'e', 0x79}, {'E', 0x79},
	{'f', 0x71}, {'F', 0x71}
};

uint8_t led_get_code(char ch)
{
	uint8_t i, bitmap = 0x00;

	for (i = 0; i < LEDMAPNUM; i++) {
		if (bcd_decode_tab[i].character == ch) {
			bitmap = bcd_decode_tab[i].bitmap;
			break;
		}
	}

	return bitmap;
}

void led_show_650(char *str, unsigned char sec_flag, unsigned char lock)
{
	int i, iLenth;
	int ret;
	int	data[4] = {0x00, 0x00, 0x00, 0x00};
	struct udevice *dev;

	if (!current_bus) {
		printf("invalid fd650 bus\n");
		return;
	}

	if (strcmp(str, "") == 0) {
		printf("invalid fd650 string\n");
		return;
	}
	iLenth = strlen(str);
	if (iLenth > 4)
		iLenth = 4;
	for (i = 0; i < iLenth; i++)
		data[i] = led_get_code(str[i]);

	fd650_write(current_bus, FD650_SYSON_8);
	fd650_write(current_bus, FD650_DIG0 | (uint8_t)data[0] | FD650_DOT);
	if (sec_flag)
		fd650_write(current_bus, FD650_DIG1 | (uint8_t)data[1] | FD650_DOT);
	else
		fd650_write(current_bus, FD650_DIG1 | (uint8_t)data[1]);
	if (lock)
		fd650_write(current_bus, FD650_DIG2 | (uint8_t)data[2] | FD650_DOT);
	else
		fd650_write(current_bus, FD650_DIG2 | (uint8_t)data[2]);
	fd650_write(current_bus, FD650_DIG3 | (uint8_t)data[3] | FD650_DOT);
}

static void *prvFD650GetInfo(void *msg)
{
	uint8_t data[5] = {0};
	uint8_t state = *(((uint8_t *)msg));
	int colon_on = *(((uint8_t *)msg) + 1);

	switch (state) {
	case FD650_STATE_SHOW:
		data[0] = *(((uint8_t *)msg) + 2);
		data[1] = *(((uint8_t *)msg) + 3);
		data[2] = *(((uint8_t *)msg) + 4);
		data[3] = *(((uint8_t *)msg) + 5);
		led_show_650(data, colon_on, 1);
		fd650_state = FD650_STATE_SHOW;
		break;
	case FD650_STATE_TIME:
		fd650_state = FD650_STATE_TIME;
		break;
	default:
		break;
	} /* end switch */

	return NULL;
}

static void fd650ShowTime(TimerHandle_t xTimer)
{
	char str[5];

	if (fd650_state == FD650_STATE_TIME) {
		get_poweroff_rtc_time(str);
		led_show_650(str, 1, 1);
	}
}

int fd650_bus_init(uint32_t id)
{
	TimerHandle_t xFD650Timer = NULL;
	int ret;

	current_bus = &fd650_plat_data[id];
	xPinmuxSet(current_bus->sda, PIN_FUNC0);
	xPinmuxSet(current_bus->scl, PIN_FUNC0);
	xGpioSetDir(current_bus->sda, GPIO_DIR_OUT);
	xGpioSetDir(current_bus->scl, GPIO_DIR_OUT);
	xGpioSetValue(current_bus->sda, GPIO_LEVEL_LOW);
	xGpioSetValue(current_bus->scl, GPIO_LEVEL_LOW);

	fd650_sda_set(current_bus, GPIO_LEVEL_HIGH);
	fd650_scl_set(current_bus, GPIO_LEVEL_HIGH);
	udelay(100);
	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_GET_FD650_INFO,
						    prvFD650GetInfo, 1);
	if (ret == MBOX_CALL_MAX) {
		iprintf("%s: mbox cmd 0x%x register fail\n", DRIVER_NAME, MBX_CMD_GET_FD650_INFO);
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	/* creat fd650 timer */
	xFD650Timer = xTimerCreate("Timer", pdMS_TO_TICKS(500), pdTRUE, NULL, fd650ShowTime);
	iprintf("%s: Starting fd650 task ...\n", DRIVER_NAME);
	xTimerStart(xFD650Timer, 0);

}
