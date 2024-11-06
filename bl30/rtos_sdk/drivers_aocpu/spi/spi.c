/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mailbox-api.h>
#include "FreeRTOS.h"
#include "common.h"
#include "semphr.h"
#include "list.h"
#include "timer_source.h"
#include "gpio.h"
#include "spi.h"

#ifdef SPI_TIME_CONSUMP
#define xHwClockSourceRead()	timere_read_us()
#endif

static List_t SpiMasterList;
static List_t SpiDeviceList;

static void prvSpiSetCs(struct SpiDevice *spi, int enable)
{
	int gpio;

	if (spi->master->is_slave || !spi->master->cs_gpios)
		return;

	if (spi->mode & SPI_CS_HIGH)
		enable = !enable;

	gpio = spi->master->cs_gpios[spi->chip_select];
	xGpioSetValue(gpio, !enable);
}

static int xAddSpiDeviceList(struct SpiDevice *device)
{
	vListInitialiseItem(&device->list_item);
	listSET_LIST_ITEM_OWNER(&device->list_item, device);
	vListInsertEnd(&SpiDeviceList, &device->list_item);

	return 0;
}

struct SpiDevice *pxSpiFindDevice(int bus_num, int chip_select)
{
	struct SpiDevice *spi;
	unsigned int i = 0;

	while (i++ < listCURRENT_LIST_LENGTH(&SpiDeviceList)) {
		listGET_OWNER_OF_NEXT_ENTRY(spi, &SpiDeviceList);
		if (spi->master->bus_num == bus_num
		    && spi->chip_select == chip_select)
			return spi;
	}

	return NULL;
}

/**
 * pxSpiNewDevice - instantiate one new SPI device
 * @master: Controller to which device is connected
 * @chip: Describes the SPI device
 * Return: the new device, or NULL.
 */
struct SpiDevice *pxSpiNewDevice(struct SpiBoardInfo *chip)
{
	struct SpiMaster *master;
	struct SpiDevice *spi;
	unsigned int i = 0;

	spi = pxSpiFindDevice(chip->bus_num, chip->chip_select);
	if (spi) {
		spi->max_speed_hz =
			spi->master->is_slave ? 0 : chip->max_speed_hz;
		spi->latency = chip->latency;
		spi->mode = chip->mode;
		spi->bits_per_word = chip->bits_per_word;
		spi_dbg("spi device %d:%d exist already\n",
			 chip->bus_num, chip->chip_select);
		return spi;
	}

	while (i++ < listCURRENT_LIST_LENGTH(&SpiMasterList)) {
		listGET_OWNER_OF_NEXT_ENTRY(master, &SpiMasterList);
		if (chip->bus_num == master->bus_num) {
			if (chip->chip_select >= master->num_chipselect) {
				spi_err("cs%d >= max %d\n", chip->chip_select,
					master->num_chipselect);
				return NULL;
			}

			spi = pvPortMalloc(sizeof(struct SpiDevice));
			if (!spi) {
				spi_err("alloc spi device failed\n");
				return NULL;
			}

			spi->master = master;
			spi->chip_select = chip->chip_select;
			spi->max_speed_hz =
				spi->master->is_slave ? 0 : chip->max_speed_hz;
			spi->latency = chip->latency;
			spi->mode = chip->mode;
			spi->bits_per_word = chip->bits_per_word;
			if (!master->is_slave && master->cs_gpios) {
				xGpioSetDir(master->cs_gpios[spi->chip_select],
						GPIO_DIR_OUT);
				prvSpiSetCs(spi, 0);
			}

			/* Add spi device to device list */
			xAddSpiDeviceList(spi);
			spi_dbg
			    ("spi %s-device @controller%d: ",
			     spi->master->is_slave ? "slave" : "master",
			     spi->master->bus_num);
			spi_dbg
			    ("mode %d, %s%s%s%s%ubits/w, %uHz\n",
			     (int)(spi->mode & (SPI_CPOL | SPI_CPHA)),
			     (spi->mode & SPI_CS_HIGH) ? "cs_high, " : "",
			     (spi->mode & SPI_LSB_FIRST) ? "lsb, " : "",
			     (spi->mode & SPI_3WIRE) ? "3wire, " : "",
			     (spi->mode & SPI_LOOP) ? "loopback, " : "",
			     spi->bits_per_word, spi->max_speed_hz);

			return spi;
		}
	}
	spi_err("can't find the controller(bus_num=%d)\n", chip->bus_num);
	return NULL;
}

void vSpiUnregisterDevice(struct SpiDevice *spi)
{
	struct SpiDevice *spi_in_list;
	int i;

	if (!spi)
		return;
	while (i++ < listCURRENT_LIST_LENGTH(&SpiDeviceList)) {
		listGET_OWNER_OF_NEXT_ENTRY(spi_in_list, &SpiDeviceList);
		if (spi == spi_in_list) {
			uxListRemove(&spi->list_item);
			vPortFree(spi);
			break;
		}
	}
}

/**
 * pvSpiAllocMaster - allocate SPI master controller
 * @size: how much zeroed driver-private data to allocate; the pointer to this
 *	memory is in the driver_data field of the returned device,
 *	accessible with pvSpiMasterGetDevData().
 */
struct SpiMaster *pvSpiAllocMaster(unsigned int size)
{
	struct SpiMaster *master;

	master = pvPortMalloc(size + sizeof(*master));
	if (!master)
		return NULL;

	memset(master, 0, size + sizeof(*master));
	master->bus_num = -1;
	master->num_chipselect = 1;
	vSpiMasterSetDevData(master, &master[1]);

	return master;
}

/**
 * xSpiRegisterMaster - register SPI master controller
 * @master: initialized master, originally from pvSpiAllocMaster()
 */
int xSpiRegisterMaster(struct SpiMaster *master)
{
	if (master->num_chipselect == 0)
		return -EINVAL;

#ifdef SPI_MUTEX
	master->mMutex = xSemaphoreCreateMutex();
	configASSERT(master->mMutex != NULL);
#endif

	if (!listLIST_IS_INITIALISED(&SpiMasterList)) {
		vListInitialise(&SpiMasterList);
		vListInitialise(&SpiDeviceList);
	}

	vListInitialiseItem(&master->list_item);
	listSET_LIST_ITEM_OWNER(&master->list_item, master);
	vListInsertEnd(&SpiMasterList, &master->list_item);

	return 0;
}

struct SpiMaster *pxSpiFindMaster(int bus_num)
{
	struct SpiMaster *master;
	unsigned int i = 0;

	while (i++ < listCURRENT_LIST_LENGTH(&SpiMasterList)) {
		listGET_OWNER_OF_NEXT_ENTRY(master, &SpiMasterList);
		if (bus_num == master->bus_num)
			return master;
	}

	return NULL;
}

/*
 * prvSpiTransferOneMessage
 *
 * This is a standard implementation of transfer_one_message() for
 * drivers which implement a transfer_one() operation.  It provides
 * standard handling of delays and chip select management.
 */
static int prvSpiTransferOneMessage(struct SpiMaster *master,
				    struct SpiMessage *msg)
{
	struct SpiTransfer *xfer;
	int keep_cs = 0;
	int ret = 0;
	int i;
#ifdef SPI_TIME_CONSUMP
	u32 xfer_time, msg_time = (u32)xHwClockSourceRead();
#endif

	if (master->prepare)
		master->prepare(master, msg->spi);

	prvSpiSetCs(msg->spi, 1);

	for (i = 0; i < msg->num_xfers; i++) {
		xfer = &msg->xfers[i];
		if (xfer->len && (xfer->tx_buf || xfer->rx_buf)) {
			if ((xfer->len * 8) % msg->spi->bits_per_word) {
				spi_err("unmatched length %d and bw %d\n",
					xfer->len, msg->spi->bits_per_word);
				return -EINVAL;
			}
#ifdef SPI_TIME_CONSUMP
			xfer_time = (u32)xHwClockSourceRead();
#endif
			ret = master->transfer_one(master, msg->spi, xfer);
#ifdef SPI_TIME_CONSUMP
			xfer->time_consump = (u32)xHwClockSourceRead();
			xfer->time_consump -= xfer_time;
#endif
			if (ret < 0) {
				spi_err("SPI transfer failed: %d\n", ret);
				goto out;
			}
		}

		if (!master->is_slave) {
			if (xfer->delay_usecs)
				udelay(xfer->delay_usecs);

			if (xfer->cs_change) {
				if (i + 1 == msg->num_xfers)
					/* last transfer */
					keep_cs = 1;
				else {
					prvSpiSetCs(msg->spi, 0);
					udelay(10);
					prvSpiSetCs(msg->spi, 1);
				}
			}
		}
		msg->actual_length += xfer->len;
	}

 out:
	if (!master->is_slave && (ret != 0 || !keep_cs))
		prvSpiSetCs(msg->spi, 0);

	if (master->unprepare)
		master->unprepare(master);

#ifdef SPI_TIME_CONSUMP
	msg->time_consump = (u32)xHwClockSourceRead() - msg_time;
#endif

	return ret;
}

void vSpiMessageInit(struct SpiMessage *msg, struct SpiTransfer *xfers,
		     int num_xfers)
{
	//memset((void *)msg, 0, sizeof(*msg));
	msg->xfers = xfers;
	msg->num_xfers = num_xfers;
}

int xSpiSync(struct SpiDevice *spi, struct SpiMessage *msg)
{
	struct SpiMaster *master = spi->master;

	msg->spi = spi;
	return prvSpiTransferOneMessage(master, msg);
}

struct MbSpiTransfer {
	uint8_t bus_num;
	uint8_t chip_select;
	uint8_t use_dma;
	struct SpiTransfer xfer;
};

static void *prvMbSpiNewDevice(void *data)
{
	struct SpiBoardInfo *chip = (struct SpiBoardInfo *)data;
	struct SpiDevice *spi;

	spi = pxSpiNewDevice(chip);
	if (!spi)
		spi_dbg("%s: new mbspi device failed\n", __func__);

	return NULL;
}

static void *prvMbSpiXfer(void *data)
{
	struct SpiDevice *spi;
	struct SpiMessage msg;
	struct MbSpiTransfer mbxfer;
	struct SpiTransfer *t;
	int *ret = (int *)data;

	mbxfer = *(struct MbSpiTransfer *)data;
	spi = pxSpiFindDevice(mbxfer.bus_num, mbxfer.chip_select);
	if (!spi) {
		spi_err("mbspi device unavailable\n");
		*ret = -ENODEV;
		return NULL;
	}

	if ((mbxfer.use_dma && spi->bits_per_word != 64)
	    || (!mbxfer.use_dma && spi->bits_per_word == 64)) {
		spi_err("mbspi device unmatched bits_per_word\n");
		*ret = -EIO;
		return NULL;
	}

	t = &mbxfer.xfer;
	if (!mbxfer.use_dma) {
		if (t->tx_buf)
			t->tx_buf = ((struct MbSpiTransfer *)data) + 1;
		if (t->rx_buf)
			t->rx_buf = pvPortMalloc(t->len);
	}

	vSpiMessageInit(&msg, t, 1);
	*ret = xSpiSync(spi, &msg);

	if (!mbxfer.use_dma && t->rx_buf) {
		if (!*ret)
			memcpy(&ret[1], t->rx_buf, t->len);
		vPortFree(t->rx_buf);
	}

	return NULL;
}

void vMbSpiInit(void)
{
	int ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL,
						    MBX_CMD_SPI_DEV,
						    prvMbSpiNewDevice, 1);
	spi_dbg("register MBX_CMD_SPI_DEV %s\n",
		(ret == MBOX_CALL_MAX) ? "failed" : "success");

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL,
						    MBX_CMD_SPI_XFER,
						    prvMbSpiXfer, 1);
	spi_dbg("register MBX_CMD_SPI_XFER %s\n",
		(ret == MBOX_CALL_MAX) ? "failed" : "success");
}
