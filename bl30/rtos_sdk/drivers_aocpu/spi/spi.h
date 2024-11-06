/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SPI_H
#define __SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#define spi_err printf
#define spi_dbg printf

#if defined(HIFI) || defined(ARCH_CPU_M4)
#define SPI_AXI_SRAM_AVAILABLE
#else
#define is_in_ddr(mem)		1
#define is_in_axi_sram(mem)	0
#endif

#define SPI_MALLOC_DEFAULT	0
#define SPI_MALLOC_AXI_SRAM	1

//#define SPI_MUTEX
#define SPI_TIME_CONSUMP

/* SPI mode */
#define	SPI_CPHA	0x01	/* clock phase */
#define	SPI_CPOL	0x02	/* clock polarity */
#define	SPI_MODE_0	(0|0)	/* (original MicroWire) */
#define	SPI_MODE_1	(0|SPI_CPHA)
#define	SPI_MODE_2	(SPI_CPOL|0)
#define	SPI_MODE_3	(SPI_CPOL|SPI_CPHA)
#define	SPI_CS_HIGH	0x04	/* chipselect active high? */
#define	SPI_LSB_FIRST	0x08	/* per-word bits-on-wire */
#define	SPI_3WIRE	0x10	/* SI/SO signals shared */
#define	SPI_LOOP	0x20	/* loopback mode */
#define	SPI_NO_CS	0x40	/* 1 dev/bus, no chipselect */
#define	SPI_READY	0x80	/* slave pulls low to pause */
#define	SPI_TX_DUAL	0x100	/* transmit with 2 wires */
#define	SPI_TX_QUAD	0x200	/* transmit with 4 wires */
#define	SPI_RX_DUAL	0x400	/* receive with 2 wires */
#define	SPI_RX_QUAD	0x800	/* receive with 4 wires */

#define MESON_GXL_SPICC		0
#define MESON_TXL_SPICC		1
#define MESON_TXLX_SPICC	1
#define MESON_AXG_SPICC		2
#define MESON_G12A_SPICC	3
#define MESON_G12B_SPICC	3
#define MESON_S5_SPICC		4
#define MESON_T5M_SPICC		4
#define MESON_TXHD2_SPICC	4

	struct SpiccHwPlatformData {
		int16_t compatible;
		unsigned long reg;
		int irq;
		int is_slave;
		int force_ssctl;
		uint32_t clk_rate;
		uint8_t bus_num;
		uint8_t num_chipselect;
		const int *cs_gpios;
	};


/**
 * struct SpiBoardInfo - board-specific template for a SPI device
 * @platform_data: Initializes SpiDevice.platform_data; the particular
 *	data stored there is driver-specific.
 * @max_speed_hz: Initializes SpiDevice.max_speed_hz; based on limits
 *	from the chip datasheet and board-specific signal quality issues.
 * @bus_num: Identifies which SpiMaster parents the SpiDevice; unused
 *	by pxSpiNewDevice(), and otherwise depends on board wiring.
 * @chip_select: Initializes SpiDevice.chip_select; depends on how
 *	the board is wired.
 * @mode: Initializes SpiDevice.mode; based on the chip datasheet, board
 *	wiring (some devices support both 3WIRE and standard modes), and
 *	possibly presence of an inverter in the chipselect path.
 */
	struct SpiBoardInfo {
		const void *platform_data;
		uint8_t bus_num;
		uint8_t chip_select;
		uint32_t max_speed_hz;
		int32_t latency;
		uint16_t mode;
		uint8_t bits_per_word;
	};

	struct SpiDevice;
	struct SpiMessage;
	struct SpiTransfer;

/**
 * struct SpiMaster - interface to SPI master controller
 * @bus_num: board-specific (and often SOC-specific) identifier for a
 *	given SPI controller.
 * @num_chipselect: chipselects are used to distinguish individual
 *	SPI slaves, and are numbered from zero to num_chipselects.
 *	each slave has a chipselect signal, but it's common that not
 *	every chipselect is connected to a slave.
 * @cs_gpios: Array of GPIOs to use as chip select lines; one per CS
 *	number. Any individual value may be -ENOENT for CS lines that
 *	are not GPIOs (driven by the SPI controller itself).
 * @setup: updates the device mode and clocking records used by a
 *	device's SPI controller; protocol code may call this.  This
 *	must fail if an unrecognized or unsupported mode is requested.
 *	It's always safe to call this unless transfers are pending on
 *	the device whose settings are being modified.
_* @transfer_one: transfer a single SpiTransfer.
 *	- return 0 if the transfer is finished,
 *	- return 1 if the transfer is still in progress.
 *	When the driver is finished with this transfer it must call
 *	spi_finalize_current_transfer() so the subsystem can issue
 *	the next transfer.
 * @cleanup: frees controller-specific state
 * @state: 0-idle, 1-busy
 */
	struct SpiMaster {
#ifdef SPI_MUTEX
		xSemaphoreHandle mMutex;
#endif
		int is_slave;
		void *dev_data;
		ListItem_t list_item;
		uint8_t bus_num;
		uint8_t num_chipselect;
		const int *cs_gpios;
		int (*prepare)(struct SpiMaster *master,
			       struct SpiDevice *spi);
		int (*unprepare)(struct SpiMaster *master);
		int (*transfer_one)(struct SpiMaster *master,
				    struct SpiDevice *spi,
				    struct SpiTransfer *transfer);
	};

/**
 * struct SpiDevice - Master side proxy for an SPI slave device
 * @master: SPI controller used with the device.
 * @max_speed_hz: Maximum clock rate to be used with this chip
 *	(on this board); may be changed by the device's driver.
 *	The SpiTransfer.speed_hz can override this for each transfer.
 * @mode: The spi mode defines how data is clocked out and in.
 *	This may be changed by the device's driver.
 *	The "active low" default for chipselect mode can be overridden
 *	(by specifying SPI_CS_HIGH) as can the "MSB first" default for
 *	each word in a transfer (by specifying SPI_LSB_FIRST).
 * @bits_per_word: Data transfers involve one or more words; word sizes
 *	like eight or 12 bits are common.  In-memory wordsizes are
 *	powers of two bytes (e.g. 20 bit samples use 32 bits).
 *	This may be changed by the device's driver, or left at the
 *	default (0) indicating protocol words are eight bit bytes.
 *	The SpiTransfer.bits_per_word can override this for each transfer.
 * @chip_select: Chipselect, distinguishing chips handled by @master.
 * @state: 0-idle, 1-busy
 */
	struct SpiDevice {
		struct SpiMaster *master;
		ListItem_t list_item;
		uint8_t chip_select;
		uint32_t max_speed_hz;
		int latency;
		uint16_t mode;
		uint8_t bits_per_word;
		int state;
	};

/**
 * struct SpiTransfer - a read/write buffer pair
 * @tx_buf: data to be written (dma-safe memory), or NULL
 * @rx_buf: data to be read (dma-safe memory), or NULL
 * @len: size of rx and tx buffers (in bytes)
 * @cs_change: affects chipselect after this transfer completes
 * @delay_usecs: microseconds to delay after this transfer before
 *	(optionally) changing the chipselect status, then starting
 *	the next transfer or completing this @SpiMessage.

 */
	struct SpiTransfer {
		const void *tx_buf;
		void *rx_buf;
		uint32_t len;
		uint32_t cs_change:1;
		uint16_t delay_usecs;
#ifdef SPI_TIME_CONSUMP
		uint32_t time_consump;
#endif
	};

/**
 * struct SpiMessage - one multi-segment SPI transaction
 * @transfers: list of transfer segments in this transaction
 * @actual_length: the total number of bytes that were transferred in all
 *	successful segments
 * @status: zero for success, else negative errno
 */
	struct SpiMessage {
		struct SpiDevice *spi;
		struct SpiTransfer *xfers;
		int num_xfers;
		unsigned int actual_length;
		int status;
#ifdef SPI_TIME_CONSUMP
		uint32_t time_consump;
#endif
	};

	extern struct SpiMaster *pvSpiAllocMaster(unsigned int size);
	extern int xSpiRegisterMaster(struct SpiMaster *master);
	extern struct SpiMaster *pxSpiFindMaster(int bus_num);
	extern struct SpiDevice *pxSpiNewDevice(struct SpiBoardInfo *info);
	extern void vSpiUnregisterDevice(struct SpiDevice *spi);
	extern void vSpiMessageInit(struct SpiMessage *msg,
				    struct SpiTransfer *xfers,
				    int num_xfers);
	extern int xSpiSync(struct SpiDevice *spi, struct SpiMessage *msg);

	static inline void *pvSpiMasterGetDevData(struct SpiMaster *master)
	{
		return master->dev_data;
	}

	static inline void *pvSpiMasterGetDevDataByBusNum(int bus_num)
	{
		struct SpiMaster *master = pxSpiFindMaster(bus_num);

		return master ? pvSpiMasterGetDevData(master) : NULL;
	}

	static inline void vSpiMasterSetDevData(struct SpiMaster *master,
						void *data)
	{
		master->dev_data = data;
	}

	extern int xSpiccProbe(struct SpiccHwPlatformData *pdata);
	extern void vSpicc1Init(void);
	extern void vSpiMasterTask(void *pvParameter);
	extern void vMbSpiInit(void);

#ifdef __cplusplus
}
#endif
#endif				/* __SPI_H */
