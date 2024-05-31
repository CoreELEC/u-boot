/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __HDMIRX_WAKE__
#define __HDMIRX_WAKE__
/* t3x register address */
#define HDMIRX_PHY_BASE_T3X	0xFE39C000
#define HDMIRX_PHY_OFFSET_T3X	0x400
#define HDMIRX_SINGLE_TOP_BASE_T3X	0xFE398000
#define HDMIRX_SINGLE_TOP_OFFSET_T3X	0x400
#define HDMIRX_COMMON_TOP_BASE_T3X	0xFE399800

/* T3X HDMI2.0 PHY register */
#define T3X_HDMIRX20PLL_CTRL0			(0x000 << 2)
#define T3X_HDMIRX20PLL_CTRL1			(0x001 << 2)
#define T3X_HDMIRX20PHY_DCHA_AFE		(0x002 << 2)
#define T3X_HDMIRX20PHY_DCHA_DFE		(0x003 << 2)
#define T3X_HDMIRX20PHY_DCHD_CDR		(0x004 << 2)
#define T3X_HDMIRX20PHY_DCHD_EQ			(0x005 << 2)
#define T3X_HDMIRX20PHY_DCHA_MISC1		(0x006 << 2)
#define T3X_HDMIRX20PHY_DCHA_MISC2		(0x007 << 2)
/* T3X HDMI2.1 PHY register */
#define T3X_HDMIRX21PHY_MISC0           (0x45 << 2)
#define T3X_HDMIRX21PHY_MISC1           (0x46 << 2)
#define T3X_HDMIRX21PHY_MISC2           (0x47 << 2)
#define T3X_HDMIRX21PHY_DCHA_AFE        (0x48 << 2)
#define T3X_HDMIRX21PHY_DCHA_DFE        (0x49 << 2)
#define T3X_HDMIRX21PHY_DCHA_PI         (0x4a << 2)
#define T3X_HDMIRX21PHY_DCHA_CTRL       (0x4b << 2)
#define T3X_HDMIRX21PHY_DCHD_CDR        (0x4c << 2)
#define T3X_HDMIRX21PHY_DCHD_EQ         (0x4d << 2)

/* TOP registers */
#define TOP_SW_RESET                     0x000
#define HDMIRX_TOP_SW_RESET_COMMON			0x00
#define TOP_HPD_PWR5V                    0x002
#define	TOP_MISC_STAT0_T3X				0x083

enum port_sts_e {
	E_PORT0,
	E_PORT1,
	E_PORT2,
	E_PORT3,
};

#define INFO(fmt, args...) printf("[%s] " fmt "\n", __func__, ##args)
#define TOP_EDID_RAM_OVR0_DATA    ((volatile uint32_t *)(0xfe398000 + (0x016 << 2)))
void hdmirx_GpioIRQRegister(void);
void hdmirx_GpioIRQFree(void);
void rx_wr_reg_b(unsigned long reg_addr, unsigned int val);
unsigned int rx_rd_reg_b(unsigned long reg_addr);
void hdmirx_wr_single_top(unsigned long addr, unsigned long data, unsigned char port);
unsigned int hdmirx_rd_single_top(unsigned long reg_addr, unsigned char port);
unsigned int hdmirx_rd_common_top(unsigned long reg_addr);
void hdmirx_wr_common_top(unsigned long addr, unsigned long data);
unsigned long hdmirx_rd_amlphy_t3x(unsigned long addr, unsigned char port);
void hdmirx_wr_amlphy_t3x(unsigned long addr, unsigned long data, unsigned char port);
void aml_phy_cfg_t3x_20(unsigned char port);
void aml_phy_cfg_t3x_21(unsigned char port);
void hdmirx_phy_init(void);
void top_common_init(void);
void hdmirx_init(void);
void vHDMIRX_task(void *pvParameters);

#endif
