// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <errno.h>
#include <env.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <amlogic/cpu_id.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/pinctrl_init.h>
#include <linux/sizes.h>
#include <asm-generic/gpio.h>
#include <dm.h>
#include <amlogic/aml_v3_burning.h>
#include <amlogic/aml_v2_burning.h>
#include <linux/mtd/partitions.h>
#include <asm/amlogic/arch/bl31_apis.h>
#include <amlogic/aml_mtd.h>
#include <amlogic/board.h>
#include <asm-generic/u-boot.h>
#include <command.h>
#include <asm/amlogic/arch/usb.h>
#include <asm/amlogic/arch/romboot.h>
#include <asm/amlogic/arch/stick_mem.h>
#include <fs.h>
#include <blk.h>
#include <mmc.h>

#ifdef CONFIG_AML_VPU
#include <amlogic/media/vpu/vpu.h>
#endif
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#ifdef CONFIG_AML_HDMITX20
#include <amlogic/media/vout/hdmitx/hdmitx_module.h>
#endif
#ifdef CONFIG_AML_HDMITX21
#include <amlogic/media/vout/hdmitx21/hdmitx_module.h>
#endif
#ifdef CONFIG_AMLOGIC_AMFC
#include <amlogic/amfc.h>
#endif
#include <asm/amlogic/arch/efuse.h>

DECLARE_GLOBAL_DATA_PTR;
extern int cc_statue, bc_status;

int mmc_get_env_dev(void);

void sys_led_init(void)
{
	run_command("gpio set GPIODV_5", 0);
	run_command("gpio clear GPIODV_6", 0);
}

int serial_set_pin_port(unsigned long port_base)
{
	return 0;
}

int dram_init(void)
{
	gd->ram_size = (readl(SYSCTRL_SEC_STATUS_REG4) & ~0xffffUL) << 4;
	return 0;
}

/* secondary_boot_func
 * this function should be write with asm, here, is only for compiling pass
 */
void secondary_boot_func(void)
{
}

int board_eth_init(bd_t *bis)
{
	return 0;
}

#ifdef CONFIG_AML_HDMITX20
static void hdmitx_set_hdmi_5v(void)
{
	/*Power on VCC_5V for HDMI_5V */
}
#endif

void board_init_mem(void)
{
	/* config bootm low size, make sure whole dram/psram space can be used */
	phys_size_t ram_size;
	char *env_tmp;

	env_tmp = env_get("bootm_size");
	if (!env_tmp) {
		ram_size =
		    ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4) >
		    0xe0000000 ? 0xe0000000 : ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4);
		env_set_hex("bootm_low", 0);
		env_set_hex("bootm_size", ram_size);
	}
}

int eth_get_efuse_mac(struct udevice *dev) {
	char buf[32], str[37];
	loff_t offset = 0;
	int ret, i, n = 0;

	memset(buf, 0, sizeof(buf));
	ret = efuse_read_usr(buf, 32, (loff_t *)&offset);
	if (ret != 32) {
		printf("ERROR: efuse read user data fail!\n");
		return -1;
	}
	for (i = 0; i < 16; ++i) {
		if (buf[i + 16] != 0) {
			n = 16;
			break;
		}
	}

	sprintf(str, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		buf[n + 0], buf[n + 1], buf[n + 2], buf[n + 3],
		buf[n + 4], buf[n + 5], buf[n + 6], buf[n + 7],
		buf[n + 8], buf[n + 9], buf[n + 10], buf[n + 11],
		buf[n + 12], buf[n + 13], buf[n + 14], buf[n + 15]
	);
	printf("board id: %s\n", str);
	env_set("serial#", str);

	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
		buf[n + 10], buf[n + 11], buf[n + 12], buf[n + 13], buf[n + 14], buf[n + 15]
	);
	printf("mac address: %s\n", str);
	env_set("ethaddr", str);

	return 0;
}

int board_init(void)
{
	printf("board init\n");

	sys_led_init();

#ifdef CONFIG_AML_HDMITX21
	hdmitx21_chip_type_init(MESON_CPU_ID_S7D);
	hdmitx21_init();
#endif

#if !defined(CONFIG_PXP_DDR)	//bypass below operations for pxp
	aml_set_bootsequence(0);

	run_command("gpio set GPIOH_7", 0);
#ifdef CONFIG_AML_HDMITX20
	hdmitx_set_hdmi_5v();
	hdmitx_init();
#endif
#endif // #if !defined(CONFIG_PXP_DDR) //bypass below operations for pxp

	pinctrl_devices_active(PIN_CONTROLLER_NUM);
#ifdef CONFIG_AMLOGIC_AMFC
	amfc_init();
#endif
	return 0;
}

static int load_from_mmc(unsigned long addr, int devnum, int partnum, char *filename)
{
	int ret;
	char buf[16];

	snprintf(buf, sizeof(buf), "%d:%d", devnum, partnum);

	ret = fs_set_blk_dev("mmc", buf, FS_TYPE_ANY);
	if (!ret) {
		loff_t len_read;
		ret = fs_read(filename, addr, 0, 0, &len_read);
		if (!ret) {
			env_set_ulong("filesize", len_read);
			printf("%llu bytes read\n", len_read);
			return 0;
		}
	}

	return ret;
}

int load_odroid_bios(void)
{
	const char *bootcmd = "cramfsload $loadaddr boot.scr; source $loadaddr";
	unsigned long addr;
	int n;
	int ret;
	struct blk_desc *dev_desc;
	struct mmc *mmc = find_mmc_device(mmc_get_env_dev());
	if (!mmc)
		return -EIO;

	dev_desc = mmc_get_blk_desc(mmc);
	if (!dev_desc)
		return -EIO;

	addr = env_get_hex("cramfsaddr", 0);
	if (!addr) {
		addr = 0x38000000;
		env_set_hex("cramfsaddr", addr);
	}

	/* Clear memory at $crarmfsaddr */
	memset((void*)addr, 0, 256);

	for (n = 1; n <= 3; n++) {
		ret = load_from_mmc(addr, dev_desc->devnum, n, "ODROIDBIOS.BIN");
		if (!ret) {
			env_set_hex("devnum", dev_desc->devnum);
			env_set("bootcmd", bootcmd);
			return 0;
		}
	}

	return -EIO;
}

int board_late_init(void)
{
	unsigned char chipid[16];

	board_init_mem();

	get_stick_reboot_flag_mbx();

#ifdef CONFIG_AML_VPU
	vpu_probe();
#endif
#ifdef CONFIG_AML_VPP
	vpp_init();
#endif
	run_command("showlogo", 0);

	memset(chipid, 0, 16);
	env_set("cpu_id", "1234567890");
	if (get_chip_id(chipid, 16) != -1) {
		char chipid_str[32];
		int i, j;
		char buf_tmp[4];

		memset(chipid_str, 0, 32);

		char *buff = &chipid_str[0];

		for (i = 0, j = 0; i < 12; ++i) {
			sprintf(&buf_tmp[0], "%02x", chipid[15 - i]);
			if (strcmp(buf_tmp, "00") != 0) {
				sprintf(buff + j, "%02x", chipid[15 - i]);
				j = j + 2;
			}
		}
		env_set("cpu_id", chipid_str);
		printf("buff: %s\n", buff);
	}

	if (load_odroid_bios() != 0) {
		env_set("mmc_list",
				(mmc_get_env_dev() == 0) ? "0 1" : "1 0");
	}

	return 0;
}

phys_size_t get_effective_memsize(void)
{
	// >>16 -> MB, <<20 -> real size, so >>16<<20 = <<4
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	return ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4) > 0xe0000000 ? 0xe0000000 :
	    (((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4) - CONFIG_SYS_MEM_TOP_HIDE);
#else
	return ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4) > 0xe0000000 ? 0xe0000000 :
	    ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4);
#endif /* CONFIG_SYS_MEM_TOP_HIDE */
}

int mach_cpu_init(void)
{
	//printf("\nmach_cpu_init\n");
	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	/* eg: bl31/32 rsv */
	return 0;
}

int __attribute__((weak)) mmc_initialize(bd_t *bis)
{
	return 0;
}

//int __attribute__((weak)) do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
//{
//	return 0;
//}

void __attribute__((weak)) set_working_fdt_addr(ulong addr)
{
}

//int __attribute__((weak)) ofnode_read_u32_default(ofnode node, const char *propname, u32 def)
//{
//	return 0;
//}

void __attribute__((weak)) md5_wd(unsigned char *input, int len, unsigned char output[16],
				  unsigned int chunk_sz)
{
}

const char *boot_device_name(int n)
{
        struct names {
                int id;
                const char* name;
        } names[] = {
                { BOOT_ID_RESERVED, "RESERVED" },
                { BOOT_ID_EMMC, "EMMC" },
                { BOOT_ID_NAND, "NAND" },
                { BOOT_ID_SPI, "SPI" },
                { BOOT_ID_SDCARD, "SD" },
                { BOOT_ID_USB, "USB" },
        };
        int i;

        for (i = 0; i < ARRAY_SIZE(names); i++)
                if (names[i].id == n)
                        return names[i].name;

        return NULL;
}

int get_boot_device(void)
{
	return (readl(SYSCTRL_SEC_STATUS_REG2) >> 4) & 0xf;
}

int mmc_get_env_dev(void)
{
	return (get_boot_device() == BOOT_ID_SDCARD) ? 1 : 0;
}
