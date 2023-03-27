/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "common.h"
#include "uart.h"
#include "FreeRTOS.h"
#include <task.h>
#include "meson_i2c_slave.h"
#include <i2c_slave_plat.h>

#define BIT(nr)			(1UL << (nr))
#define GENMASK(h, l) \
(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

/* Control register fields */
#define REG_CTRL_STATE_EN		BIT(7)
#define REG_CTRL_STATUS			BIT(26)
#define REG_CTRL_IRQ_EN			BIT(25)
#define REG_CTRL_ACK_EN			BIT(24)
#define REG_CTRL_SEND_EN		BIT(28)
#define REG_CTRL_RECV_EN		BIT(27)
#define REG_CTRL_ADDR_SHIFT		16
#define REG_CTRL_HOLD_TIME_SHIFT	8

#define I2C_TIMEOUT_MS                  (1000 * 100)
#define DRIVER_NAME				"[MESON I2C Slv]"
// #define I2C_SLAVE_DEBUG			1
#ifdef I2C_SLAVE_DEBUG
#define IIC_DBG(fmt, ...) printf("%s," fmt, DRIVER_NAME, ##__VA_ARGS__)
#else
#define IIC_DBG(fmt, ...)
#endif

struct xI2cSlaveRegs {
	uint32_t ctrl;
	uint32_t send;
	uint32_t recv;
	uint32_t cntl1;
};

struct xMesonI2cSlave {
	struct xI2cSlaveRegs *regs;
	uint32_t irq;		/* IRQ Number */
	uint32_t slave_addr;
	unsigned long reset_reg;
	uint32_t reset_bit;
};

struct xMesonI2cSlave slave_i2cs[4];

uint32_t current_slave_id;

static void prvSetBitsLe32(uint32_t *reg, uint32_t set)
{
	uint32_t val;

	val = REG32(reg);
	val |= (set);
	REG32(reg) = val;
}

static void prvClrBitsLe32(uint32_t *reg, uint32_t clr)
{
	uint32_t val;

	val = REG32(reg);
	val &= (~(clr));
	REG32(reg) = val;
}

#if I2C_SLAVE_DEBUG
static void prvClrSetBitsLe32(uint32_t *reg, uint32_t clr, uint32_t set)
{
	uint32_t val;

	val = REG32(reg);
	val &= (~(clr));
	val |= (set);
	REG32(reg) = val;
}

void xI2cSlaveDumpRegs(void)
{
	printf("i2c slave reg : 0x%x = 0x%x\n", &slave_i2cs[current_slave_id].regs->ctrl,
		slave_i2cs[current_slave_id].regs->ctrl);
	printf("i2c slave reg : 0x%x = 0x%x\n", &slave_i2cs[current_slave_id].regs->send,
		slave_i2cs[current_slave_id].regs->send);
	printf("i2c slave reg : 0x%x = 0x%x\n", &slave_i2cs[current_slave_id].regs->recv,
		slave_i2cs[current_slave_id].regs->recv);
	printf("i2c slave reg : 0x%x = 0x%x\n", &slave_i2cs[current_slave_id].regs->cntl1,
		slave_i2cs[current_slave_id].regs->cntl1);
}
#endif

int32_t xI2cSlaveInit(struct xMesonI2cSlavePlatdata *plat, int normal)
{
	struct xI2cSlaveRegs *regs = (struct xI2cSlaveRegs *)plat->reg;
	uint32_t reset_reg = plat->reset_reg;
	uint32_t reset_bit = plat->reset_bit;

	REG32((uint32_t *)reset_reg) = 1 << reset_bit;
	prvClrBitsLe32(&regs->ctrl, 0xffffffff);
	if (normal)
		prvSetBitsLe32(&regs->cntl1, 0x3f);
	else {/*for rtc wake up*/
		prvClrBitsLe32(&regs->cntl1, 0xffff);
		prvSetBitsLe32(&regs->cntl1, 0x8000);
	}
	prvSetBitsLe32(&regs->ctrl, REG_CTRL_STATE_EN |
							REG_CTRL_ACK_EN |
							REG_CTRL_IRQ_EN |
							REG_CTRL_SEND_EN |
							REG_CTRL_RECV_EN);
	prvSetBitsLe32(&regs->ctrl, 0x0 << REG_CTRL_HOLD_TIME_SHIFT);
	prvSetBitsLe32(&regs->ctrl, 0x0 << 0);
	prvSetBitsLe32(&regs->ctrl, (plat->slave_addr << 1) <<
							REG_CTRL_ADDR_SHIFT);

	return 0;
}
/*
 *i2c slave platform data init
 */
int32_t xI2cSlaveMesonInit(uint32_t id, int normal)
{
	struct xMesonI2cSlavePlatdata *plat = NULL;

	plat = &i2cSlaveData[id];
	slave_i2cs[id].regs = (struct xI2cSlaveRegs *)plat->reg;
	slave_i2cs[id].irq = plat->irq;
	slave_i2cs[id].slave_addr = plat->slave_addr;
	slave_i2cs[id].reset_reg = plat->reset_reg;
	slave_i2cs[id].reset_bit = plat->reset_bit;
	current_slave_id = id;
	IIC_DBG("index = %u, reg = 0x%lx, irq = %u, slave_addr = %x\n",
		plat->index, plat->reg, slave_i2cs[id].irq, plat->slave_addr);
	IIC_DBG("reset reg = 0x%lx, bit = %d\n", plat->reset_reg, plat->reset_bit);
	xI2cSlaveInit(plat, normal);
	IIC_DBG("ctrl = 0x%x\n", REG32(&(slave_i2cs[current_slave_id].regs->ctrl)));
	IIC_DBG("send = 0x%x\n", REG32(&(slave_i2cs[current_slave_id].regs->send)));
	IIC_DBG("receive = 0x%x\n", REG32(&(slave_i2cs[current_slave_id].regs->recv)));
	IIC_DBG("cntl1 = 0x%x\n", REG32(&(slave_i2cs[current_slave_id].regs->cntl1)));

	return 0;
}
