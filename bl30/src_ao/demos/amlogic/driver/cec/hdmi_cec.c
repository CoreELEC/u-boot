/**************************************************
 *           HDMI CEC uboot code                  *
 *                                                *
 **************************************************/
/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include "myprintf.h"
#include "common.h"
#include "util.h"
#include <unistd.h>
#include "irq.h"
//#include "n200_eclic.h"
#include "n200_func.h"
#include "projdefs.h"
#include "portmacro.h"
#include "suspend.h"
#include "mailbox-api.h"
#include "rpc-user.h"

#define CONFIG_CEC_WAKEUP

#ifdef CONFIG_CEC_WAKEUP
#include "cec-data.h"
#include "hdmi_cec.h"
#include "hdmi_cec_reg.h"
#ifndef NULL

#define NULL ((void *)0)
#endif

#define CEC_REG_DEBUG		0
#define CEC_CFG_DEBUG		0
#define CEC_FW_DEBUG		0
#define CEC_USE_IRQ		0/*not work*/

#define PHY_ADDR_LEN		4 /*16bit/4bit*/

#define DEVICE_TV		0
#define DEVICE_RECORDER		1
#define DEVICE_RESERVED		2
#define DEVICE_TUNER		3
#define DEVICE_PLAYBACK		4
#define DEVICE_AUDIO_SYSTEM	5
#define DEVICE_PURE_CEC_SWITCH	6
#define DEVICE_VIDEO_PROCESSOR	7

typedef struct {
	unsigned int wk_logic_addr:8;
	unsigned int wk_phy_addr:16;
	unsigned int wk_port_id:8;
}cec_wakeup_t;

struct st_cec_mailbox_data {
	unsigned int cec_config;
	unsigned int phy_addr;
	unsigned int vendor_id;
	unsigned char osd_name[16];
} __packed;

static unsigned int cec_wait_addr;
static cec_wakeup_t cec_wakup;
static unsigned char hdmi_cec_func_config;
static cec_msg_t cec_msg;
static u32 cec_wakup_flag;
struct st_cec_mailbox_data cec_mailbox;
static enum cec_chip_ver cec_chip = CEC_CHIP_SC2;

struct cec_tx_msg_t {
	unsigned char buf[16];
	unsigned char retry;
	unsigned char len;
};

#define CEX_TX_MSG_BUF_NUM	  4
#define CEC_TX_MSG_BUF_MASK	 (CEX_TX_MSG_BUF_NUM - 1)

struct cec_tx_msg {
	struct cec_tx_msg_t msg[CEX_TX_MSG_BUF_NUM];
	unsigned char send_idx;
	unsigned char queue_idx;
};

struct cec_tx_msg cec_tx_msgs = {};


static void cec_reset_addr(void);

void cec_delay(u32 cnt)
{
	volatile u32 tmp = cnt;

	while (tmp-- > 0) {
		tmp = tmp;
	}
}

static u32 set_cec_val0(unsigned int cec_val)
{
	/*printf("%s warning: is empty,sts:0x%x\n", __func__, cec_val);*/
	cec_val = cec_val;
	return 0;
}

static u32 set_cec_val1(unsigned int cec_val)
{
	/*printf("%s warning: is empty,sts:0x%x\n", __func__, cec_val);*/
	cec_val = cec_val;
	return 0;
}

static void cec_get_portinfo(void *msg)
{
	u32 val;

	val = cec_wakup.wk_logic_addr | (cec_wakup.wk_phy_addr << 8) |
		(cec_wakup.wk_port_id << 24);
	*(u32 *)msg = val;
	printf("[%s]: info=0x%x\n", __func__, val);
}

static int cec_strlen(char *p)
{
	int i = 0;

	while (*p++)
		i++;
	return i;
}

static void *cec_memcpy(void *memto, const void *memfrom, unsigned int size)
{
	char *tempfrom = (char *)memfrom;
	char *tempto = (char *)memto;

	if ((memto == NULL) || (memfrom == NULL))
		return NULL;
	while (size -- > 0)
		*tempto++ = *tempfrom++;
	return memto;
}

void cec_update_config_data(void *data)
{
	unsigned int i;

	memcpy((void *)&cec_mailbox, data, sizeof(struct st_cec_mailbox_data));

	if (cec_mailbox.cec_config & CEC_CFG_DBG_EN) {
		printf("cec_config:0x%x\n", cec_mailbox.cec_config);
		printf("phy_addr:0x%x\n", cec_mailbox.phy_addr);
		printf("vendor_id:0x%x\n", cec_mailbox.vendor_id);
		printf("osd_name:");
		for (i = 0; i < 16; i++) {
			printf(" 0x%x", cec_mailbox.osd_name[i]);
		}
		printf("\n");
	}
}

void cec_update_phyaddress(unsigned int phyaddr)
{
	cec_mailbox.phy_addr = cec_mailbox.phy_addr & 0xffff0000;
	cec_mailbox.phy_addr |= phyaddr & 0xffff;
	printf("update phyaddr:0x%x\n", phyaddr);
}

void cec_update_func_cfg(unsigned int cfg)
{
	cec_mailbox.cec_config = cfg;

	if (hdmi_cec_func_config & CEC_CFG_DBG_EN) {
		printf("cec_config:0x%x\n", cfg);
	}
}

static void write_ao(unsigned int addr, unsigned int data)
{
	unsigned int real_addr;

	real_addr = cec_reg_tab[addr];

	if (real_addr == 0xffff) {
		printf("w ao no exist reg:0x%x", real_addr);
		return;
	}
#if CEC_FW_DEBUG
	//printf("%s :0x%x val:0x%x\n", __func__, real_addr, data);
#endif
	REG32(real_addr) = data;
}

static unsigned int read_ao(unsigned int addr)
{
	unsigned int real_addr;
	unsigned int data;

	real_addr = cec_reg_tab[addr];

	if (real_addr == 0xffff) {
		printf("r ao no exist reg:0x%x", real_addr);
		return 0x0;
	}

	data = REG32(real_addr);
#if CEC_FW_DEBUG
	//printf("%s :0x%x val:0x%x\n", __func__, real_addr, data);
#endif
	return data;
}

static unsigned long cecb_rd_reg(unsigned long addr)
{
	unsigned long data32;
	unsigned int timeout = 0;

	data32  = 0;
	data32 |= 0    << 16;  // [16]   cec_reg_wr
	data32 |= 0    << 8;   // [15:8] cec_reg_wrdata
	data32 |= addr << 0;   // [7:0]  cec_reg_addr
	//REG32(CECB_RW_REG) = data32;
	write_ao(CECB_REG_RW_REG, data32);
	while (data32 & (1 << 23)) {
		if (timeout++ > 500) {
			printf("cecb r reg 0x%x fail\n",
				(unsigned int)addr);
			break;
		}
		data32 = read_ao(CECB_REG_RW_REG);/*REG32(CECB_RW_REG);*/
	}
	data32 = ((read_ao(CECB_REG_RW_REG)) >> 24) & 0xff;
	return (data32);
} /* cecb_rd_reg */

static void cecb_wr_reg (unsigned long addr, unsigned long data)
{
	unsigned long data32;
	unsigned int timeout = 0;

	//data32 = REG32(CECB_RW_REG);
	data32 = read_ao(CECB_REG_RW_REG);
	while (data32 & (1 << 23)) {
		if (timeout++ > 200) {
			printf("cecb w reg 0x%x fail\n",
				(unsigned int)addr);
			break;
		}
		/*data32 = REG32(CECB_RW_REG);*/
		data32 = read_ao(CECB_REG_RW_REG);
	}

	data32  = 0;
	data32 |= 1 << 16;  // [16]   cec_reg_wr
	data32 |= data << 8;   // [15:8] cec_reg_wrdata
	data32 |= addr << 0;   // [7:0]  cec_reg_addr
	//REG32(CECB_RW_REG) = data32;
	write_ao(CECB_REG_RW_REG, data32);
} /* aocec_wr_only_reg */

static inline void cec_set_bits_dwc(uint32_t reg, uint32_t bits,
				       uint32_t start, uint32_t len)
{
	unsigned int tmp;
	tmp = cecb_rd_reg(reg);
	tmp &= ~(((1 << len) - 1) << start);
	tmp |=  (bits << start);
	cecb_wr_reg(reg, tmp);
}

static void cec_set_reg_bits(unsigned long addr, unsigned int value,
	unsigned int offset, unsigned int len)
{
	unsigned int data32 = 0;

	data32 = read_ao(addr);//REG32(addr);
	data32 &= ~(((1 << len) - 1) << offset);
	data32 |= (value & ((1 << len) - 1)) << offset;
	//REG32(addr) = data32;
	write_ao(addr, data32);
}

static void cec_rx_read_pos_plus(void)
{
	(cec_msg.rx_read_pos ==  cec_msg.rx_buf_size - 1) ?
				(cec_msg.rx_read_pos = 0) :
				(cec_msg.rx_read_pos++);
}

#if CEC_REG_DEBUG
static void dump_cecb_reg(void)
{
	int i = 0;
	unsigned char reg;
	unsigned int reg32;

	reg32 = read_ao(CECB_REG_CLK_CNTL_REG0);
	printf("CECB_CLK_CNTL0:0x%x\n", reg32);
	reg32 = read_ao(CECB_REG_CLK_CNTL_REG1);
	printf("CECB_CLK_CNTL1:0x%x\n", reg32);
	reg32 = read_ao(CECB_REG_GEN_CNTL);
	printf("CECB_GEN_CNTL:0x%x\n", reg32);
	reg32 = read_ao(CECB_REG_RW_REG);
	printf("CECB_RW:0x%x\n", reg32);
	reg32 = read_ao(CECB_REG_INTR_MASKN);
	printf("CECB_INT_MASKN:0x%x\n", reg32);
	reg32 = read_ao(CECB_REG_INTR_STAT);
	printf("CECB_INT_STAT:0x%x\n", reg32);

	printf("CECB_CTRL:0x%x\n", cecb_rd_reg(DWC_CECB_CTRL));
	printf("CECB_CTRL2:0x%x\n", cecb_rd_reg(DWC_CECB_CTRL2));
	printf("CECB_MASK:0x%x\n", cecb_rd_reg(DWC_CECB_INTR_MASK));
	printf("CECB_ADDR_L:0x%x\n", cecb_rd_reg(DWC_CECB_LADD_LOW));
	printf("CECB_ADDR_H:0x%x\n", cecb_rd_reg(DWC_CECB_LADD_HIGH));
	printf("CECB_TX_CNT:0x%x\n", cecb_rd_reg(DWC_CECB_TX_CNT));
	printf("CECB_RX_CNT:0x%x\n", cecb_rd_reg(DWC_CECB_RX_CNT));
	printf("CECB_STAT0:0x%x\n", cecb_rd_reg(DWC_CECB_STAT0));
	printf("CECB_LOCK:0x%x\n", cecb_rd_reg(DWC_CECB_LOCK_BUF));
	printf("CECB_WKUPCTRL:0x%x\n", cecb_rd_reg(DWC_CECB_WAKEUPCTRL));

	printf("CECB_RX buffer:");
	for (i = 0; i < 16; i++) {
		reg = cecb_rd_reg(DWC_CECB_RX_DATA00 + i);
		printf(" 0x%x", reg);
	}
	printf("\n");
	printf("CECB_TX buffer:");
	for (i = 0; i < 16; i++) {
		reg = cecb_rd_reg(DWC_CECB_TX_DATA00 + i);
		printf(" 0x%x", reg);
	}
	printf("\n");
}
#endif

static void cec_enable_irq(u32 onoff)
{
	if (onoff) {
		/*enable the interrupt*/
		/*REG32(CECB_INTR_MASKN) = CECB_IRQ_EN_MASK;*/
		write_ao(CECB_REG_INTR_MASKN, CECB_IRQ_EN_MASK);
		cecb_wr_reg(DWC_CECB_WAKEUPCTRL, WAKEUP_DIS_MASK);
	} else {
		/*REG32(CECB_INTR_MASKN) = 0;*/
		write_ao(CECB_REG_INTR_MASKN, 0);
		cecb_wr_reg(DWC_CECB_WAKEUPCTRL, WAKEUP_DIS_MASK);
	}
}

static void cec_clear_int_sts(void)
{
	unsigned int reg;

	/*reg = REG32(CECB_INTR_STAT);*/
	/*REG32(CECB_INTR_CLR) = reg;*/
	reg = read_ao(CECB_REG_INTR_STAT);
	if (reg)
		write_ao(CECB_REG_INTR_STAT, reg);
}

#if 0
static void cec_sts_check(void)
{
	printf("CECB_INTR_STAT=0x%x\n", read_ao(CECB_REG_INTR_STAT));
	printf("DWC_CECB_LOCK_BUF=0x%x\n", cecb_rd_reg(DWC_CECB_LOCK_BUF));
	printf("DWC_CECB_CTRL=0x%x\n", cecb_rd_reg(DWC_CECB_CTRL));
}
#endif

static u32 cec_set_pin_mux(u32 chip)
{
	xPinmuxSet(CEC_PIN_MX, CEC_PIN_FUNC);
}

static u32 cec_hw_reset(void)
{
	unsigned int reg;
	unsigned int data32;

	reg =   (0 << 31) |
		(0 << 30) |
		(1 << 28) |		/* clk_div0/clk_div1 in turn */
		((732-1) << 12) |	/* Div_tcnt1 */
		((733-1) << 0);		/* Div_tcnt0 */
	/*REG32(CLKCTRL_CECB_CTRL0) = reg;*/
	write_ao(CECB_REG_CLK_CNTL_REG0, reg);
	reg =   (0 << 13) |
		((11-1)  << 12) |
		((8-1)  <<  0);
	/*REG32(CLKCTRL_CECB_CTRL1) = reg;*/
	write_ao(CECB_REG_CLK_CNTL_REG1, reg);

	reg = read_ao(CECB_REG_CLK_CNTL_REG0);/*REG32(CLKCTRL_CECB_CTRL0);*/
	reg |= (1 << 31);
	/*REG32(CLKCTRL_CECB_CTRL0) = reg;*/
	write_ao(CECB_REG_CLK_CNTL_REG0, reg);

	/*_udelay(200);*/
	reg |= (1 << 30);
	/*REG32(CLKCTRL_CECB_CTRL0) = reg;*/
	write_ao(CECB_REG_CLK_CNTL_REG0, reg);

	data32  = 0;
	data32 |= (7 << 12);	/* filter_del */
	data32 |= (1 <<  8);	/* filter_tick: 1us */
	data32 |= (1 <<  3);	/* enable system clock */
	data32 |= 0 << 1;	/* [2:1]	cntl_clk: */
				/* 0=Disable clk (Power-off mode); */
				/* 1=Enable gated clock (Normal mode); */
				/* 2=Enable free-run clk (Debug mode). */
	data32 |= 1 << 0;	/* [0]	  sw_reset: 1=Reset */
	/*REG32(CECB_GEN_CNTL) = data32;*/
	write_ao(CECB_REG_GEN_CNTL, data32);
	/* Enable gated clock (Normal mode). */
	cec_set_reg_bits(CECB_REG_GEN_CNTL, 1, 1, 1);
	/* Release SW reset */
	cec_set_reg_bits(CECB_REG_GEN_CNTL, 0, 0, 1);

	reg = 0;
	reg |= (0 << 6);/*curb_err_init*/
	reg |= (0 << 5);/*en_chk_sbitlow*/
	reg |= (2 << 0);/*rise_del_max*/
	cecb_wr_reg(DWC_CECB_CTRL2, reg);

	cec_set_pin_mux(cec_chip);

	cec_clear_int_sts();
	/*enable the interrupt*/
	cec_enable_irq(1);
	cec_delay(200);
	return 0;
}

static unsigned char remote_cec_ll_rx(void)
{
	int i;
	int len;

	len = cecb_rd_reg(DWC_CECB_RX_CNT);
	printf("cec R:");
	for (i = 0; i < len; i++) {
		cec_msg.buf[cec_msg.rx_write_pos].msg[i] = cecb_rd_reg(DWC_CECB_RX_DATA00 + i);
		printf(" 0x%02x", cec_msg.buf[cec_msg.rx_write_pos].msg[i]);
	}
	/* clr CEC lock bit */
	cecb_wr_reg(DWC_CECB_LOCK_BUF, 0);
	cec_msg.buf[cec_msg.rx_write_pos].msg_len = len;
	printf("\n");
	return 0;
}
static void cec_buf_clear(void)
{
	int i;

	for (i = 0; i < 16; i++)
		cec_msg.buf[cec_msg.rx_read_pos].msg[i] = 0;
}

static void cec_tx_buf_init(void)
{
	int i, j;
	for (j = 0; j < CEX_TX_MSG_BUF_NUM; j++) {
		for (i = 0; i < 16; i++) {
			cec_tx_msgs.msg[j].buf[i] = 0;
		}
		cec_tx_msgs.msg[j].retry = 0;
		cec_tx_msgs.msg[j].len = 0;
	}
}

static int cec_queue_tx_msg(unsigned char *msg, unsigned char len)
{
	int s_idx, q_idx;

	s_idx = cec_tx_msgs.send_idx;
	q_idx = cec_tx_msgs.queue_idx;
	if (((q_idx + 1) & CEC_TX_MSG_BUF_MASK) == s_idx) {
		printf("tx buffer full, abort msg\n");
		cec_reset_addr();
		return -1;
	}
	if (len && msg) {
		cec_memcpy(cec_tx_msgs.msg[q_idx].buf, msg, len);
		cec_tx_msgs.msg[q_idx].len = len;
		cec_tx_msgs.queue_idx = (q_idx + 1) & CEC_TX_MSG_BUF_MASK;
	}
	return 0;
}

static int cec_triggle_tx(unsigned char *msg, unsigned char len)
{
	int i = 0, lock;

	while (1) {
		/* send is in process */
		lock = cecb_rd_reg(DWC_CECB_LOCK_BUF);
		if (lock) {
			printf("rx msg in tx\n");
			return -1;
		}
		if (cecb_rd_reg(DWC_CECB_CTRL) & 0x01)
			i++;
		else
			break;
		if (i > 25) {
			printf("lock:0x%x, wait busy timeout\n", lock);
			return -1;
		}
		cec_delay(50);
	}

	printf("cec T:");
	for (i = 0; i < len; i++) {
		cecb_wr_reg(DWC_CECB_TX_DATA00 + i, msg[i]);
		printf(" 0x%02x", msg[i]);
	}
	printf("\n");

	/* start send */
	cecb_wr_reg(DWC_CECB_TX_CNT, len);
	cec_set_bits_dwc(DWC_CECB_CTRL, 3, 0, 3);
	return 0;
}

static int remote_cec_ll_tx(unsigned char *msg, unsigned char len)
{
	int ret = 0;

	cec_queue_tx_msg(msg, len);
	ret = cec_triggle_tx(msg, len);

	return ret;
}


static int cec_check_irq_sts(void)
{
	unsigned int reg;
	unsigned int ret = TX_IDLE;
	unsigned int cnt = 0;

	while (cec_tx_msgs.queue_idx != cec_tx_msgs.send_idx) {
		/*reg = REG32(CECB_INTR_STAT);*/
		/*REG32(CECB_INTR_CLR) = reg;*/
		reg = read_ao(CECB_REG_INTR_STAT);
		if (reg)
			write_ao(CECB_REG_INTR_CLR, reg);

		if (reg & CECB_IRQ_TX_DONE) {
			ret = TX_DONE;
			cec_tx_msgs.send_idx = (cec_tx_msgs.send_idx + 1) & CEC_TX_MSG_BUF_MASK;
			printf("tx:TX_DONE %d\n", cnt);
			break;
		}

		if (reg & CECB_IRQ_TX_NACK) {
			ret = TX_ERROR;
			cec_tx_msgs.send_idx = (cec_tx_msgs.send_idx + 1) & CEC_TX_MSG_BUF_MASK;
			printf("tx:TX_NACK %d\n", cnt);
			break;
		}
		if (reg & CECB_IRQ_TX_ARB_LOST) {
			ret = TX_BUSY;
			cec_tx_msgs.send_idx = (cec_tx_msgs.send_idx + 1) & CEC_TX_MSG_BUF_MASK;
			printf("tx:TX_ABT_LOST %d\n", cnt);
			break;
		}
		if (reg & CECB_IRQ_TX_ERR_INITIATOR) {
			ret = TX_BUSY;
			cec_tx_msgs.send_idx = (cec_tx_msgs.send_idx + 1) & CEC_TX_MSG_BUF_MASK;
			printf("tx:TX_ERR_INIT %d\n", cnt);
			break;
		}

		if (cnt++ >= 200) {
			printf("%s time out %d\n", __func__, cnt);
			cnt = 0;
			break;
		}
		cec_delay(200);
	}

	return ret;
}

static int ping_cec_ll_tx(unsigned char *msg, unsigned char len)
{
	unsigned int ret = 0;

	/*printf("ping: 0x%x\n", msg[0]);*/
	ret = remote_cec_ll_tx(msg, len);
	return ret;
}

static unsigned char log_addr_to_devtye(unsigned int addr)
{
	static unsigned char addr_map[] = {
		DEVICE_TV,
		DEVICE_RECORDER,
		DEVICE_RECORDER,
		DEVICE_TUNER,
		DEVICE_PLAYBACK,
		DEVICE_AUDIO_SYSTEM,
		DEVICE_TUNER,
		DEVICE_TUNER,
		DEVICE_PLAYBACK,
		DEVICE_RECORDER,
		DEVICE_TUNER,
		DEVICE_PLAYBACK,
		DEVICE_RESERVED,
		DEVICE_RESERVED,
		DEVICE_TV
	};
	return addr_map[addr & 0xf];
}

static void cec_report_physical_address(void)
{
	unsigned char msg[5];

	msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_BROADCAST_ADDR;
	msg[1] = CEC_OC_REPORT_PHYSICAL_ADDRESS;
	msg[2] = (cec_mailbox.phy_addr >> 8) & 0xff;
	msg[3] = cec_mailbox.phy_addr & 0xff;
	msg[4] = log_addr_to_devtye(cec_msg.log_addr);

	remote_cec_ll_tx(msg, 5);
}

static void cec_report_device_power_status(int dst)
{
	unsigned char msg[3];

	msg[0] = ((cec_msg.log_addr & 0xf) << 4)| (dst & 0xf);
	msg[1] = CEC_OC_REPORT_POWER_STATUS;
	msg[2] = cec_msg.power_status;

	remote_cec_ll_tx(msg, 3);
}

static void cec_set_stream_path(void)
{
	unsigned char phy_addr_ab = (cec_mailbox.phy_addr >> 8) & 0xff;
	unsigned char phy_addr_cd = cec_mailbox.phy_addr & 0xff;

	if ((hdmi_cec_func_config >> CEC_FUNC_MASK) & 0x1) {
		if ((hdmi_cec_func_config >> AUTO_POWER_ON_MASK) & 0x1) {
			if ((phy_addr_ab == cec_msg.buf[cec_msg.rx_read_pos].msg[2]) &&
			    (phy_addr_cd == cec_msg.buf[cec_msg.rx_read_pos].msg[3]))  {
				cec_msg.cec_power = 0x1;
				cec_msg.active_source = 1;
				printf("%s power on\n", __func__);
			}
		}
	}
}

static int cec_routing_change(void)
{
	unsigned char phy_addr_ab = (cec_mailbox.phy_addr >> 8) & 0xff;
	unsigned char phy_addr_cd = cec_mailbox.phy_addr & 0xff;

	if ((hdmi_cec_func_config >> CEC_FUNC_MASK) & 0x1) {
		if ((hdmi_cec_func_config >> AUTO_POWER_ON_MASK) & 0x1) {
			/* wake up if routing destination is self */
			if ((phy_addr_ab == cec_msg.buf[cec_msg.rx_read_pos].msg[4]) &&
			    (phy_addr_cd == cec_msg.buf[cec_msg.rx_read_pos].msg[5])) {
				cec_msg.cec_power = 0x1;
				cec_msg.active_source = 1;
				printf("%s power on\n", __func__);
			}
		}
	}
	return cec_msg.cec_power;
}

static void cec_device_vendor_id(void)
{
	unsigned char msg[5];
	unsigned int vendor_id = cec_mailbox.vendor_id;

	msg[0] = ((cec_msg.log_addr & 0xf) << 4)| CEC_BROADCAST_ADDR;
	msg[1] = CEC_OC_DEVICE_VENDOR_ID;
	msg[2] = (vendor_id >> 16) & 0xff;
	msg[3] = (vendor_id >> 8) & 0xff;
	msg[4] = (vendor_id >> 0) & 0xff;

	remote_cec_ll_tx(msg, 5);
}

static void cec_menu_status_smp(int menu_status, int dst)
{
	unsigned char msg[3];

	msg[0] = ((cec_msg.log_addr & 0xf) << 4)| (dst & 0xf);
	msg[1] = CEC_OC_MENU_STATUS;
	msg[2] = menu_status;

	remote_cec_ll_tx(msg, 3);
}

static void cec_give_deck_status(int dst)
{
	unsigned char msg[3];

	msg[0] = ((cec_msg.log_addr & 0xf) << 4) | (dst & 0xf);
	msg[1] = CEC_OC_DECK_STATUS;
	msg[2] = 0x1a;

	remote_cec_ll_tx(msg, 3);
}

/*static void cec_standby(void)
{
	unsigned char msg[2];

	msg[0] = ((cec_msg.log_addr & 0xf) << 4) | CEC_BROADCAST_ADDR;
	msg[1] = CEC_OC_STANDBY;

	remote_cec_ll_tx(msg, 2);
}*/

static void cec_set_osd_name(int dst)
{
	unsigned char msg[16];
	unsigned char osd_len = cec_mailbox.osd_name[15];

	msg[0] = ((cec_msg.log_addr & 0xf) << 4) | (dst & 0xf);
	msg[1] = CEC_OC_SET_OSD_NAME;
	if (osd_len > 0 && osd_len <= 14) {
		cec_memcpy(&msg[2], cec_mailbox.osd_name, osd_len);
	} else {
		osd_len = cec_strlen(CONFIG_CEC_OSD_NAME);
		cec_memcpy(&msg[2], CONFIG_CEC_OSD_NAME, osd_len);
	}

	remote_cec_ll_tx(msg, osd_len + 2);
}

static void cec_get_version(int dst)
{
	unsigned char dest_log_addr = cec_msg.log_addr & 0xf;
	unsigned char msg[3];

	if (0xf != dest_log_addr) {
		msg[0] = ((cec_msg.log_addr & 0xf) << 4) | (dst & 0xf);
		msg[1] = CEC_OC_CEC_VERSION;
		msg[2] = CEC_VERSION_14A;
		remote_cec_ll_tx(msg, 3);
	}
}

static int check_addr(int phy_addr)
{
	unsigned int local_addr = (cec_mailbox.phy_addr) & 0xffff;
	unsigned int i, mask = 0xf000, a, b;

	for (i = 0; i < 4; i++) {
		if (!(local_addr & mask)) {
			break;
		}
		a = local_addr & mask;
		b = phy_addr & mask;
		if (a != b)	{// node is not same
			printf("addr fail 1\n");
			return 0;
		}
		mask >>= 4;
	}
	printf("addr ok\n");
	return 1;
}

static int is_playback_dev(int addr)
{
	if (addr != CEC_PLAYBACK_DEVICE_1_ADDR &&
	    addr != CEC_PLAYBACK_DEVICE_2_ADDR &&
	    addr != CEC_PLAYBACK_DEVICE_3_ADDR) {
		return 0;
	}
	return 1;
}

#if 0
static int is_phy_addr_ready(cec_msg_t *msg)
{
	if (msg == NULL)
		return 0;
	if (((hdmi_cec_func_config >> CEC_FUNC_MASK) & 0x1) &&
	    ((hdmi_cec_func_config >> AUTO_POWER_ON_MASK) & 0x1) &&
	    msg->cec_power) {
		if (cec_wakup.wk_phy_addr &&
		    cec_wakup.wk_logic_addr &&
			(cec_wakup.wk_phy_addr != 0xFFFF)) {
				printf("is_phy_addr_ready 0x%x\n",
				       cec_wakup.wk_phy_addr);
				return 1;
		}
	}

	return 0;
}
#endif

static u32 cec_save_port_id(void)
{
	unsigned int phy_addr;
	int i, port_id;
	u32 data;

	phy_addr = cec_wakup.wk_phy_addr;
	printf("save port id\n");
	if ((phy_addr == 0xFFFF) || ((phy_addr & 0xF000) == 0)) {
		cec_wakup.wk_port_id = 0xFF;
		data = cec_wakup.wk_logic_addr | (cec_wakup.wk_phy_addr << 8) |
			(cec_wakup.wk_port_id << 24);
		set_cec_val1(data);
		return 0;
	}

	for (i = 0; i < PHY_ADDR_LEN; i++) {
		port_id = (phy_addr >> (PHY_ADDR_LEN - i - 1)*4) & 0xF;
		if (port_id == 0) {
			 port_id = (phy_addr >> (PHY_ADDR_LEN - i)*4) & 0xF;
			 break;
		}
	}
	cec_wakup.wk_port_id = port_id;
	data = cec_wakup.wk_logic_addr | (cec_wakup.wk_phy_addr << 8) |
		(cec_wakup.wk_port_id << 24);
	set_cec_val1(data);
	return 0;
}

static u32 cec_handle_message(void)
{
	unsigned char opcode;
	unsigned char source;
	unsigned int  phy_addr;
	u32 data;

	source = (cec_msg.buf[cec_msg.rx_read_pos].msg[0] >> 4) & 0xf;
	if ((hdmi_cec_func_config & CEC_CFG_FUNC_EN) &&
		(cec_msg.buf[cec_msg.rx_read_pos].msg_len > 1)) {
		opcode = cec_msg.buf[cec_msg.rx_read_pos].msg[1];
#if CEC_FW_DEBUG
		printf("handl:0x%02x\n", opcode);
#endif
		switch (opcode) {
		case CEC_OC_GET_CEC_VERSION:
			cec_get_version(source);
			break;
		case CEC_OC_GIVE_DECK_STATUS:
			cec_give_deck_status(source);
			break;
		case CEC_OC_GIVE_PHYSICAL_ADDRESS:
			cec_report_physical_address();
			break;
		case CEC_OC_GIVE_DEVICE_VENDOR_ID:
			cec_device_vendor_id();
			break;
		case CEC_OC_GIVE_OSD_NAME:
			cec_set_osd_name(source);
			break;
		case CEC_OC_SET_STREAM_PATH:
			cec_set_stream_path();
			break;
		case CEC_OC_ROUTING_CHANGE:
			cec_routing_change();
			break;
		case CEC_OC_GIVE_DEVICE_POWER_STATUS:
			cec_report_device_power_status(source);
			break;
		case CEC_OC_USER_CONTROL_PRESSED:
			if (((hdmi_cec_func_config >> CEC_FUNC_MASK) & 0x1) &&
			    ((hdmi_cec_func_config >> AUTO_POWER_ON_MASK) & 0x1) &&
			     (cec_msg.buf[cec_msg.rx_read_pos].msg_len == 3) &&
			    ((0x40 == cec_msg.buf[cec_msg.rx_read_pos].msg[2]) ||
			     (0x6d == cec_msg.buf[cec_msg.rx_read_pos].msg[2]) ||
			     (0x09 == cec_msg.buf[cec_msg.rx_read_pos].msg[2]) )) {
				cec_msg.cec_power = 0x1;
				printf("user power on\n");
			}
			break;
		case CEC_OC_MENU_REQUEST:
			cec_menu_status_smp(DEVICE_MENU_INACTIVE, source);
			break;

		/* TV Wake up by image/text view on */
		case CEC_OC_IMAGE_VIEW_ON:
		case CEC_OC_TEXT_VIEW_ON:
			if (((hdmi_cec_func_config >> CEC_FUNC_MASK) & 0x1) &&
			    ((hdmi_cec_func_config >> AUTO_POWER_ON_MASK) & 0x1) &&
			    (!is_playback_dev(cec_msg.log_addr))) {
				/* request active source needed */
				phy_addr = 0xffff;
				cec_msg.cec_power = 0x1;
				cec_wakup.wk_logic_addr = source;
				cec_wakup.wk_phy_addr = phy_addr;
				data = cec_wakup.wk_logic_addr | (cec_wakup.wk_phy_addr << 8) |
					(cec_wakup.wk_port_id << 24);
				set_cec_val1(data);
				printf("otp power on\n");
			}
			break;

		/* TV Wake up by active source*/
		case CEC_OC_ACTIVE_SOURCE:
			phy_addr = (cec_msg.buf[cec_msg.rx_read_pos].msg[2] << 8) |
				   (cec_msg.buf[cec_msg.rx_read_pos].msg[3] << 0);
			if (((hdmi_cec_func_config >> CEC_FUNC_MASK) & 0x1) &&
			    ((hdmi_cec_func_config >> AUTO_POWER_ON_MASK) & 0x1) &&
			    (!is_playback_dev(cec_msg.log_addr) && check_addr(phy_addr))) {
				cec_msg.cec_power = 0x1;
				cec_msg.active_source = 1;
				cec_wakup.wk_logic_addr = source;
				cec_wakup.wk_phy_addr = phy_addr;
				data = cec_wakup.wk_logic_addr | (cec_wakup.wk_phy_addr << 8) |
					(cec_wakup.wk_port_id << 24);
				set_cec_val1(data);
				printf("active src power on:0x%x\n", data);
			}
			break;

		default:
			break;
		}
	}
	cec_rx_read_pos_plus();
	return 0;
}

static void cec_set_log_addr(int addr)
{
	cecb_wr_reg(DWC_CECB_LADD_LOW, 0);
	cecb_wr_reg(DWC_CECB_LADD_HIGH, 0x00);
	if (addr > 15)
		return ;
	if ((addr & 0x0f) < 8)
		cecb_wr_reg(DWC_CECB_LADD_LOW, 1 << addr);
	else
		cecb_wr_reg(DWC_CECB_LADD_HIGH, (1 << (addr - 8))/* | 0x80*/);

	cec_delay(100);
}

static void cec_reset_addr(void)
{
	int addr = cec_msg.log_addr;

	cec_hw_reset();
	cec_set_log_addr(addr);
}

static unsigned char cec_get_log_addr(void)
{
	int i, reg;

	reg = cecb_rd_reg(DWC_CECB_LADD_LOW);
	reg = (cecb_rd_reg(DWC_CECB_LADD_HIGH) << 8) | reg;
	for (i = 0; i < 16; i++) {
		if (reg & (1 << i))
			break;
	}
	if (reg & 0x8000 && i < 16)
		return i + 16;
	else if (i < 16)
		return i;
	return 0xff;
}

static u32 cec_irq_handler(void)
{
	unsigned char s_idx;
	static int busy_count = 0;
	int irq;

	/*irq = REG32(CECB_INTR_STAT);*/
	/*REG32(CECB_INTR_CLR) = irq;*/
	irq = read_ao(CECB_REG_INTR_STAT);
	if (irq)
		write_ao(CECB_REG_INTR_CLR, irq);
	else
		return 0;

#if CEC_REG_DEBUG
	if (irq) {
		printf("irq sts:0x%x\n", irq);
		dump_cecb_reg();
	}
#endif
	if (irq & CECB_IRQ_RX_EOM) {
		remote_cec_ll_rx();
		(cec_msg.rx_write_pos == cec_msg.rx_buf_size - 1) ? (cec_msg.rx_write_pos = 0) : (cec_msg.rx_write_pos++);
#if CEC_REG_DEBUG
		printf("RX_OK\n");
#endif
	}
	if (irq & CECB_IRQ_RX_ERR_FOLLOWER) {
		printf("RX_ERROR\n");
		cecb_wr_reg(DWC_CECB_LOCK_BUF, 0);
	}
	if (irq & CECB_IRQ_RX_WAKEUP) {
		printf("rx wake up\n");
		cecb_wr_reg(DWC_CECB_WAKEUPCTRL, 0);
		/* TODO: wake up system if needed */
	}

	if (irq & CECB_IRQ_TX_DONE) {
		cec_tx_msgs.send_idx = (cec_tx_msgs.send_idx + 1) & CEC_TX_MSG_BUF_MASK;
		s_idx = cec_tx_msgs.send_idx;
		if (cec_tx_msgs.send_idx != cec_tx_msgs.queue_idx) {
			printf("TX_OK\n");
			cec_triggle_tx(cec_tx_msgs.msg[s_idx].buf,
				       cec_tx_msgs.msg[s_idx].len);
		} else {
#if CEC_REG_DEBUG
			printf("TX_END\n");
#endif
		}
		busy_count = 0;
	}

	if (irq & CECB_IRQ_TX_NACK) {
		printf("@TX_NACK\n");
		s_idx = cec_tx_msgs.send_idx;
		if (cec_tx_msgs.msg[s_idx].retry < 2) {
			cec_tx_msgs.msg[s_idx].retry++;
			cec_triggle_tx(cec_tx_msgs.msg[s_idx].buf,
				       cec_tx_msgs.msg[s_idx].len);
		} else {
			/*printf("TX retry too much, abort msg\n");*/
			cec_tx_msgs.send_idx = (cec_tx_msgs.send_idx + 1) & CEC_TX_MSG_BUF_MASK;
		}
		busy_count = 0;
	}

	if (irq & CECB_IRQ_TX_ERR_INITIATOR) {
		printf("@TX_ERR_INIT\n");
		s_idx = cec_tx_msgs.send_idx;
		if (cec_tx_msgs.send_idx != cec_tx_msgs.queue_idx) { // triggle tx if idle
			cec_triggle_tx(cec_tx_msgs.msg[s_idx].buf,
				       cec_tx_msgs.msg[s_idx].len);
		}
		busy_count = 0;
	}

	if (irq & CECB_IRQ_TX_ARB_LOST) {
	    busy_count++;
		if (busy_count >= 2000) {
			printf("busy too long, reset hw\n");
			cec_reset_addr();
			busy_count = 0;
		}
	}

	if (cec_msg.rx_read_pos != cec_msg.rx_write_pos) {
		cec_handle_message();
	}

	return 0;
}

/*static void check_standby(void)
{
	if (((cec_msg.log_addr & 0xf) == 0) &&
	    ((hdmi_cec_func_config >> CEC_FUNC_MASK) & 0x1) &&
	    ((hdmi_cec_func_config >> ONE_TOUCH_STANDBY_MASK) & 0x1)) {
		cec_standby();
	}
}*/

static unsigned int *probe = NULL;
static unsigned int ping_state = 0;/*0:send statue, 1:check irq state*/
static unsigned int idle_cnt = 0;

static void cec_node_init(void)
{
	static unsigned int retry = 0;
	/*static unsigned int regist_devs = 0;*/
	static unsigned char idx = 0, sub_idx = 0;
	unsigned int log_addr;
	int tx_stat = TX_ERROR;
	unsigned char msg[16];
	/*unsigned int kern_log_addr = (REG32(SYSCTRL_STATUS_REG1) >> 16) & 0xf;*/
	unsigned int kern_log_addr = (read_ao(CEC_REG_STS1) >> 16) & 0xf;
	unsigned int player_dev[3][3] =
		{{CEC_PLAYBACK_DEVICE_1_ADDR, CEC_PLAYBACK_DEVICE_2_ADDR, CEC_PLAYBACK_DEVICE_3_ADDR},
		 {CEC_PLAYBACK_DEVICE_2_ADDR, CEC_PLAYBACK_DEVICE_3_ADDR, CEC_PLAYBACK_DEVICE_1_ADDR},
		 {CEC_PLAYBACK_DEVICE_3_ADDR, CEC_PLAYBACK_DEVICE_1_ADDR, CEC_PLAYBACK_DEVICE_2_ADDR}};

#if CEC_FW_DEBUG
	printf("%s\n", __func__);
#endif

	cec_wait_addr = 0;
	cec_wakup_flag = 0;
	if (retry >= 12) {  // retry all device addr
		cec_msg.log_addr = 0x0f;
		printf("failed on retried all possible address\n");
		return ;
	}
	memset(&cec_wakup, 0, sizeof(cec_wakup));
	if (probe == NULL) {
		cec_msg.rx_read_pos = 0;
		cec_msg.rx_write_pos = 0;
		cec_msg.rx_buf_size = 2;

		cec_msg.power_status = 1;
		cec_msg.active_source = 0;
		cec_msg.cec_power = 0;
		cec_tx_msgs.send_idx = 0;
		cec_tx_msgs.queue_idx = 0;
		cec_msg.log_addr = 0;
		cec_tx_buf_init();
		cec_buf_clear();
		retry = 0;
		/*regist_devs = 0;*/
		idx = 0;
		/*_udelay(100);*/
		/*
		 * use kernel cec logic address to detect which logic address is the
		 * started one to allocate.
		 */
		printf("addr from kernel:0x%x\n", kern_log_addr);
		/* we don't need probe TV address */
		if (!is_playback_dev(kern_log_addr)) {
			cec_set_log_addr(kern_log_addr);
			msg[0] = (kern_log_addr << 4) | kern_log_addr;
			ping_cec_ll_tx(msg, 1);
			cec_msg.log_addr = 0x10 | kern_log_addr;
			/*_udelay(100);*/
			printf("log_addr:0x%x ADDR0:0x%x\n",
			       cec_msg.log_addr,
			       cec_get_log_addr());
			probe = NULL;
			/*regist_devs = 0;*/
			idx = 0;
			retry = 0;
			/*check_standby();*/
			return ;
		}

		for (idx = 0; idx < 3; idx++) {
			if (kern_log_addr == player_dev[idx][0]) {
				sub_idx = 0;
				probe = &player_dev[idx];
				break;
			}
		}

		if (probe == NULL) {
			probe = player_dev[0];
		} else {
			/*printf("0x%x, 0x%x, 0x%x\n", probe[0], probe[1], probe[2]);*/
		}
		sub_idx = 0;
		/*cec_hw_reset();*/
		ping_state = 0;
		idle_cnt = 0;
		retry = 0;
		return;
	} else {
		/* ping dev addr */
		log_addr = player_dev[idx][sub_idx];
		cec_set_log_addr(log_addr);
		msg[0] = (log_addr << 4 ) | log_addr;
		/*printf("%s ping:idx:%d, 0x%x\n", __func__, sub_idx, msg[0]);*/
		if (!ping_state) {
			tx_stat = ping_cec_ll_tx(msg, 1);
			cec_delay(500);
			ping_state = 1;
		} else {
			tx_stat = cec_check_irq_sts();
			if (tx_stat == TX_BUSY) {   // can't get cec bus
				printf("TX_BUSY");
				cec_hw_reset();
				if (retry++ > 4) {
					printf("TX_BUSY retry too much, log_addr:0x%x\n", probe[sub_idx]);
					retry = 0;
				}
				ping_state = 0;
				return;
			} else if (tx_stat == TX_ERROR) {
				cec_delay(100);
				/*address had allocated*/
				cec_msg.log_addr = player_dev[idx][sub_idx];
				cec_set_log_addr(cec_msg.log_addr);
				printf("Set log_addr:0x%x,addr0:0x%x\n",
				       cec_msg.log_addr, cec_get_log_addr());
				probe = NULL;
				/*regist_devs = 0;*/
				idx = 0;
				retry = 0;
				ping_state = 0;
				return ;
			} else if (tx_stat == TX_DONE) {
				printf("TX_DONE sombody takes cec log_addr:0x%x\n", player_dev[idx][sub_idx]);
				#if 0
				regist_devs |= (1 << dev_idx);
				retry += (4 - (retry & 0x03));
				if (regist_devs == 0x07) {
					// No avilable logical address
					cec_msg.log_addr = 0x0f;
					cec_set_log_addr(15);
					printf("CEC allocate logic address failed\n");
				}
				#endif
				ping_state = 0;
			} else {
				/*idle*/
				if (idle_cnt++ > 5) {
					ping_state = 0;/*to tx state*/
				} else {
					return;
				}
			}

			/*need ping next address*/
			sub_idx++;
			if (sub_idx == 3) {
				sub_idx = 0;
				/* no match addr*/
				cec_msg.log_addr = 0x0f;
				cec_set_log_addr(cec_msg.log_addr);
				printf("CEC allocate logic address failed\n");
			}
		}
	}
}

static u32 cec_suspend_wakeup_chk(void)
{
	u32 timeout_flag = 0;

	if ((cec_msg.cec_power == 0x1) &&
		(hdmi_cec_func_config & CEC_CFG_FUNC_EN)) {
		if (cec_wait_addr++ < 40) {
			if (cec_msg.active_source) {
				cec_save_port_id();
				timeout_flag = 1;
				printf("check wakeup\n");
			}
			printf(".");
		} else {
			timeout_flag = 1;
			printf("wakeup\n");
		}
	}

	if (timeout_flag) {
		cec_wakup_flag = 1;
		set_cec_val0(CEC_WAKEUP);
		return 1;
	} else {
		return 0;
	}
}

u32 cec_suspend_handle(void)
{
	u32 active_src_flag = 0;

	/*cec_sts_check();*/
	cec_suspend_wakeup_chk();
	if (cec_msg.log_addr) {
		if (hdmi_cec_func_config & CEC_CFG_FUNC_EN) {
			cec_irq_handler();
			if (cec_msg.cec_power == 0x1) {
				if (cec_msg.active_source) {
					cec_save_port_id();
					/*cec power key*/
					active_src_flag = 1;
					printf("message wakeup\n");
				}
			}
		}
	} else if (hdmi_cec_func_config & CEC_CFG_FUNC_EN) {
		cec_node_init();
	}


	if (active_src_flag) {
		printf("active source:0x%x\n", cec_msg.active_source);
		printf("wk_logic_addr:0x%x\n", cec_wakup.wk_phy_addr);
		printf("wk_phy_addr:0x%x\n", cec_wakup.wk_phy_addr);
		printf("wk_port_id:0x%x\n", cec_wakup.wk_port_id);
		return 1;
	} else
		return 0;
}

void cec_req_irq(u32 onoff)
{
#if CEC_USE_IRQ
	if (onoff) {
		//vEnableIrq(IRQ_NUM_CECB, 180);/*B:180,A:179*/
		printf("%s: TODO: please use new vEnableIiq function.\n", __func__);
		vClearPendingIrq(IRQ_NUM_CECB);
	} else {
		vDisableIrq(IRQ_NUM_CECB);/*B:180,A:179*/
	}
#if CEC_FW_DEBUG
	printf("%s %d\n", __func__, onoff);
#endif
#else
	onoff = onoff;
#endif
}

#if CEC_USE_IRQ
static void cec_handler(void)
{
	u32 irq;

	printf("%s\n");

	cec_clear_int_sts();
}
#endif

u32 cec_init_config(void)
{
	hdmi_cec_func_config = read_ao(CEC_REG_STS0);
	/*cec_mailbox.cec_config = hdmi_cec_func_config;*/
	cec_mailbox.phy_addr = read_ao(CEC_REG_STS1);

	if (cec_mailbox.cec_config & CEC_CFG_DBG_EN) {
		printf("%s\n", CEC_VERSION);
		printf("cec cfg1:0x%x\n", hdmi_cec_func_config);
		printf("cec cfg2:0x%x\n", read_ao(CEC_REG_STS1));
	}

	if (hdmi_cec_func_config & CEC_CFG_FUNC_EN) {
		cec_req_irq(1);
		probe = NULL;
		ping_state = 0;
		idle_cnt = 0;
		cec_msg.log_addr = 0;
		cec_hw_reset();
		/*cec_node_init();*/
	} else {
		cec_enable_irq(0);
	}

#if CEC_REG_DEBUG
	dump_cecb_reg();
#endif
	/*cec enable*/
	if (hdmi_cec_func_config & CEC_CFG_FUNC_EN) {
		return 1;
	} else
		return 0;
}

u32 cec_get_wakup_flag(void)
{
	return cec_wakup_flag;
}

void vCecCallbackInit(enum cec_chip_ver chip_mode)
{
	int ret;

	/*initial bl30 start call*/
	cec_mailbox.cec_config = CEC_CFG_FUNC_EN | CEC_CFG_OTP_EN | CEC_CFG_PW_ON_EN;
	cec_mailbox.phy_addr = 0x1000;

	cec_chip = chip_mode;
	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_GET_CEC_INFO,
						    cec_get_portinfo, 1);
	if (ret == MBOX_CALL_MAX)
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_GET_CEC_INFO);

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL, MBX_CMD_SET_CEC_DATA,
						    cec_update_config_data, 1);
	if (ret == MBOX_CALL_MAX)
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_SET_CEC_DATA);
}

#if CEC_USE_IRQ
DECLARE_IRQ(IRQ_NUM_CECB, cec_handler)
#endif

#endif

