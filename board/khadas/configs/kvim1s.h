/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2022 Wesion, Inc. All rights reserved.
 */

#ifndef __BOARD_CFG_H__
#define __BOARD_CFG_H__

#include <asm/arch/cpu.h>

/*
 * platform power init config
 */

#define AML_VCCK_INIT_VOLTAGE	  799	    //VCCK power up voltage
#define AML_VDDEE_INIT_VOLTAGE    800       // VDDEE power up voltage

/* SMP Definitinos */
#define CPU_RELEASE_ADDR		secondary_boot_func

/* Serial config */
#define CONFIG_CONS_INDEX 2
#define CONFIG_BAUDRATE  115200

/*if disable uboot console, enable it*/
//#define CONFIG_SILENT_CONSOLE
#ifdef CONFIG_SILENT_CONSOLE
#undef CONFIG_SILENT_CONSOLE_UPDATE_ON_RELOC
#endif

#define MCU_I2C_BUS_NUM                 1

/*low console baudrate*/
#define CONFIG_LOW_CONSOLE_BAUD			0

/* Enable ir remote wake up for bl30 */
#define AML_IR_REMOTE_POWER_UP_KEY_VAL1 0xef10fe01 //amlogic tv ir --- power
#define AML_IR_REMOTE_POWER_UP_KEY_VAL2 0XBB44FB04 //amlogic tv ir --- ch+
#define AML_IR_REMOTE_POWER_UP_KEY_VAL3 0xF20DFE01 //amlogic tv ir --- ch-
#define AML_IR_REMOTE_POWER_UP_KEY_VAL4 0XBA45BD02 //amlogic small ir--- power
#define AML_IR_REMOTE_POWER_UP_KEY_VAL5 0xe51afb04
#define AML_IR_REMOTE_POWER_UP_KEY_VAL6 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL7 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL8 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL9 0xFFFFFFFF

/*config the default parameters for adc power key*/
#define AML_ADC_POWER_KEY_CHAN   2  /*channel range: 0-7*/
#define AML_ADC_POWER_KEY_VAL    0  /*sample value range: 0-1023*/

/* Bootloader Control Block function
   That is used for recovery and the bootloader to talk to each other
  */
#ifndef CONFIG_PXP_DDR
#define CONFIG_BOOTLOADER_CONTROL_BLOCK
#endif// #ifndef CONFIG_PXP_DDR

#ifdef CONFIG_DTB_BIND_KERNEL	//load dtb from kernel, such as boot partition
#define CONFIG_DTB_LOAD  "imgread dtb ${boot_part} ${dtb_mem_addr}"
#else
#define CONFIG_DTB_LOAD  "imgread dtb _aml_dtb ${dtb_mem_addr}"
#endif//#ifdef CONFIG_DTB_BIND_KERNEL	//load dtb from kernel, such as boot partition

/* args/envs */
#define CONFIG_SYS_MAXARGS  64

#ifdef CONFIG_CMD_USB
#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_DEVICES_USB(func)
#endif

#ifndef BOOT_TARGET_DEVICES
#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_DEVICES_USB(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)
#endif

#include <config_distro_bootcmd.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	"bootcmd_spi=test.s \"$boot_source\" = \"spi\" && sf probe && "\
    "sf read $loadaddr 0x4c8000 0x8000 && script\0"\
    "fdt_addr_r=0x01000000\0"\
    "fdtoverlay_addr_r=0x00a00000\0"\
    "fdtaddr=0x01000000\0"\
    "kernel_addr_r=0x01080000\0"\
    "pxefile_addr_r=0x00010000\0"\
    "scriptaddr=0x00010000\0" \
    "ramdisk_addr_r=0x10000000\0"\
    "kernel_comp_addr_r=0x0d080000\0"\
    "kernel_comp_size=0x2000000\0"\
    "pxeuuid=00000000-0000-0000-0000-000000000000\0"\
    "bootfile=\0"\
    "fdtfile=amlogic/" CONFIG_DEFAULT_DEVICE_TREE ".dtb\0" \
        "firstboot=1\0"\
	"silent=0\0"\
        "upgrade_step=0\0"\
        "jtag=disable\0"\
        "loadaddr=0x00020000\0"\
        "os_ident_addr=0x00500000\0"\
        "loadaddr_rtos=0x00001000\0"\
        "loadaddr_kernel=0x01080000\0"\
        "dv_fw_addr=0xa00000\0"\
        "otg_device=1\0" \
        "panel_type=lcd_1\0" \
        "outputmode=1080p60hz\0" \
        "hdmimode=1080p60hz\0" \
        "colorattribute=444,8bit\0"\
        "cvbsmode=576cvbs\0" \
        "display_width=1920\0" \
        "display_height=1080\0" \
        "display_bpp=24\0" \
        "display_color_index=24\0" \
        "display_layer=osd0\0" \
        "display_color_fg=0xffff\0" \
        "display_color_bg=0\0" \
        "dtb_mem_addr=0x01000000\0" \
        "fb_addr=0x00300000\0" \
        "fb_width=1920\0" \
        "fb_height=1080\0" \
        "hdmichecksum=0x00000000\0" \
        "dolby_status=0\0" \
        "dolby_vision_on=0\0" \
        "dv_fw_dir_odm_ext=/odm_ext/firmware/dovi_fw.bin\0" \
        "dv_fw_dir_vendor=/vendor/firmware/dovi_fw.bin\0" \
        "dv_fw_dir=/reserved/firmware/dovi_fw.bin\0" \
        "frac_rate_policy=1\0" \
        "hdr_policy=0\0" \
        "usb_burning=" CONFIG_USB_TOOL_ENTRY "\0" \
        "fdt_high=0x20000000\0"\
        "sdcburncfg=aml_sdc_burn.ini\0"\
        "EnableSelinux=enforcing\0" \
        "recovery_part=recovery\0"\
        "loglevel=8\0" \
        "lock=10101000\0"\
        "recovery_offset=0\0"\
        "cvbs_drv=0\0"\
        "osd_reverse=0\0"\
        "video_reverse=0\0"\
        "active_slot=normal\0"\
        "boot_part=boot\0"\
        "vendor_boot_part=vendor_boot\0"\
        "board_logo_part=odm_ext\0" \
        "board=oppen\0"\
        "Irq_check_en=0\0"\
        "common_dtb_load=" CONFIG_DTB_LOAD "\0"\
        "get_os_type=if store read ${os_ident_addr} ${boot_part} 0 0x1000; then os_ident ${os_ident_addr}; fi\0"\
        "fatload_dev=usb\0"\
        "fs_type=""rootfstype=ramfs""\0"\
        "initargs="\
            "rootflags=data=writeback rw rootfstype=ext4" CONFIG_KNL_LOG_LEVEL "console=ttyS0,921600 console=tty0 no_console_suspend fsck.repair=yes net.ifnames=0 "\
            "ramoops.pstore_en=1 ramoops.record_size=0x8000 ramoops.console_size=0x4000 loop.max_part=4 "\
            "\0"\
        "upgrade_check="\
            "echo recovery_status=${recovery_status};"\
            "if itest.s \"${recovery_status}\" == \"in_progress\"; then "\
                "run storeargs; run recovery_from_flash;"\
            "else fi;"\
            "echo upgrade_step=${upgrade_step}; "\
            "if itest ${upgrade_step} == 3; then run storeargs; run update; fi;"\
            "\0"\
        "storeargs="\
            "get_bootloaderversion;" \
            "get_rebootmode;"\
            "setenv bootargs ${initargs} otg_device=${otg_device} "\
                "logo=${display_layer},loaded,${fb_addr} vout=${outputmode},enable panel_type=${panel_type} "\
                "hdmitx=${cecconfig},${colorattribute} hdmimode=${hdmimode} "\
                "hdmichecksum=${hdmichecksum} dolby_vision_on=${dolby_vision_on} " \
                "hdr_policy=${hdr_policy} hdr_priority=${hdr_priority} "\
                "frac_rate_policy=${frac_rate_policy} hdmi_read_edid=${hdmi_read_edid} cvbsmode=${cvbsmode} "\
                "osd_reverse=${osd_reverse} video_reverse=${video_reverse} irq_check_en=${Irq_check_en}  "\
                "androidboot.selinux=${EnableSelinux} androidboot.firstboot=${firstboot} wol_enable=${wol_enable} jtag=${jtag} reboot_mode=${reboot_mode}; "\
            "setenv bootargs ${bootargs} androidboot.bootloader=${bootloader_version} androidboot.hardware=amlogic;"\
            "run cmdline_keys;"\
            "\0"\
        "switch_bootmode="\
            "get_rebootmode;"\
            "if test ${reboot_mode} = factory_reset; then "\
                    "run recovery_from_flash;"\
            "else if test ${reboot_mode} = update; then "\
                    "run update;"\
            "else if test ${reboot_mode} = quiescent; then "\
                    "setenv bootargs ${bootargs} androidboot.quiescent=1;"\
            "else if test ${reboot_mode} = recovery_quiescent; then "\
                    "setenv bootargs ${bootargs} androidboot.quiescent=1;"\
                    "run recovery_from_flash;"\
            "else if test ${reboot_mode} = cold_boot; then "\
            "else if test ${reboot_mode} = fastboot; then "\
                "fastboot 0;"\
            "fi;fi;fi;fi;fi;fi;"\
            "\0" \
		"storeboot="\
			"kbi resetflag 0;"\
			"setenv loadaddr ${loadaddr_kernel};"\
			"usb start; "\
			"for dev in usb mmc; do "\
			"for part in 0 1 1:4; do "\
				"test -e $dev $part Image && "\
				"echo try load os from $dev $part && "\
				"load $dev $part $dtb_mem_addr dtb/$fdtfile && "\
				"load $dev $part $loadaddr Image && "\
				"load $dev $part 10000000 initrd.img && "\
				"booti $loadaddr 10000000:$filesize $dtb_mem_addr; "\
				"echo fail $dev $part;"\
			"done; "\
			"done; "\
			"\0" \
         "update="\
            /*first usb burning, second sdc_burn, third ext-sd autoscr/recovery, last udisk autoscr/recovery*/\
            "run usb_burning; "\
            "run recovery_from_sdcard;"\
            "run recovery_from_udisk;"\
            "run recovery_from_flash;"\
            "\0"\
        "recovery_from_fat_dev="\
            "setenv loadaddr ${loadaddr_kernel};"\
            "if fatload ${fatload_dev} 0 ${loadaddr} aml_autoscript; then autoscr ${loadaddr}; fi;"\
            "if fatload ${fatload_dev} 0 ${loadaddr} recovery.img; then "\
                "if fatload ${fatload_dev} 0 ${dtb_mem_addr} dtb.img; then echo ${fatload_dev} dtb.img loaded; fi;"\
                "setenv bootargs ${bootargs} ${fs_type};"\
                "bootm ${loadaddr};fi;"\
            "\0"\
        "recovery_from_udisk="\
            "setenv fatload_dev usb;"\
            "if usb start 0; then run recovery_from_fat_dev; fi;"\
            "\0"\
        "recovery_from_sdcard="\
            "setenv fatload_dev mmc;"\
            "if mmcinfo; then run recovery_from_fat_dev; fi;"\
            "\0"\
        "recovery_from_flash="\
            "echo active_slot: ${active_slot};"\
            "setenv loadaddr ${loadaddr_kernel};"\
            "if test ${active_slot} = normal; then "\
                "setenv bootargs ${bootargs} ${fs_type} aml_dt=${aml_dt} recovery_part=${recovery_part} recovery_offset=${recovery_offset};"\
				"if test ${upgrade_step} = 3; then "\
					"if ext4load mmc 1:2 ${dtb_mem_addr} /recovery/dtb.img; then echo cache dtb.img loaded; fi;"\
					"if test ${vendor_boot_mode} = true; then "\
						"if imgread kernel ${recovery_part} ${loadaddr} ${recovery_offset}; then bootm ${loadaddr}; fi;"\
					"else "\
						"if ext4load mmc 1:2 ${loadaddr} /recovery/recovery.img; then echo cache recovery.img loaded; wipeisb; bootm ${loadaddr}; fi;"\
					"fi;"\
                "else "\
            "if imgread dtb recovery ${dtb_mem_addr}; then "\
                "else echo restore dtb; run common_dtb_load;"\
            "fi;"\
                "fi;"\
            "if imgread kernel ${recovery_part} ${loadaddr} ${recovery_offset}; then bootm ${loadaddr}; fi;"\
            "else "\
                "if fdt addr ${dtb_mem_addr}; then else echo retry common dtb; run common_dtb_load; fi;"\
                "if test ${partiton_mode} = normal; then "\
                    "setenv bootargs ${bootargs} ${fs_type} aml_dt=${aml_dt} recovery_part=${boot_part} recovery_offset=${recovery_offset};"\
                    "if imgread kernel ${boot_part} ${loadaddr}; then bootm ${loadaddr}; fi;"\
                "else "\
                    "if test ${vendor_boot_mode} = true; then "\
                        "setenv bootargs ${bootargs} ${fs_type} aml_dt=${aml_dt} recovery_part=${boot_part} recovery_offset=${recovery_offset} androidboot.slot_suffix=${active_slot};"\
                        "if imgread kernel ${boot_part} ${loadaddr}; then bootm ${loadaddr}; fi;"\
                    "else "\
                        "setenv bootargs ${bootargs} ${fs_type} aml_dt=${aml_dt} recovery_part=${recovery_part} recovery_offset=${recovery_offset} androidboot.slot_suffix=${active_slot};"\
                        "if imgread kernel ${recovery_part} ${loadaddr} ${recovery_offset}; then wipeisb; bootm ${loadaddr}; fi;"\
                    "fi;"\
                "fi;"\
            "fi;"\
            "\0"\
        "bcb_cmd="\
            "get_avb_mode;"\
            "get_valid_slot;"\
            "if test ${vendor_boot_mode} = true; then "\
                "setenv loadaddr_kernel 0x3080000;"\
                "setenv dtb_mem_addr 0x1000000;"\
            "fi;"\
            "if test ${active_slot} != normal; then "\
                "echo ab mode, read dtb from kernel;"\
                "setenv common_dtb_load ""imgread dtb ${boot_part} ${dtb_mem_addr}"";"\
            "fi;"\
            "\0"\
        "load_bmp_logo="\
			"if load mmc 0:1 ${loadaddr} boot-logo-1080.bmp.gz || load mmc 1:1 ${loadaddr} boot-logo-1080.bmp.gz; then "\
			"bmp display ${loadaddr};"\
			"bmp scale;"\
			"fi;"\
            "\0"\
        "init_display="\
            "get_rebootmode;"\
            "echo reboot_mode:::: ${reboot_mode};"\
            "if test ${reboot_mode} = quiescent; then "\
                    "setenv reboot_mode_android ""quiescent"";"\
                    "setenv dolby_status 0;"\
                    "setenv dolby_vision_on 0;"\
                    "run storeargs;"\
                    "setenv bootargs ${bootargs} androidboot.quiescent=1;"\
                    "osd open;osd clear;"\
            "else if test ${reboot_mode} = recovery_quiescent; then "\
                    "setenv reboot_mode_android ""quiescent"";"\
                    "setenv dolby_status 0;"\
                    "setenv dolby_vision_on 0;"\
                    "run storeargs;"\
                    "setenv bootargs ${bootargs} androidboot.quiescent=1;"\
                    "osd open;osd clear;"\
            "else "\
                "setenv reboot_mode_android ""normal"";"\
                "run storeargs;"\
                "hdmitx hpd;hdmitx get_preferred_mode;hdmitx get_parse_edid;dovi process;osd open;osd clear;run load_bmp_logo;bmp scale;vout output ${outputmode};dovi set;dovi pkg;vpp hdrpkt;"\
            "fi;fi;"\
            "\0"\
		"storage_param="\
			"setenv bootargs ${bootargs} ${emmc_quirks}; "\
			"store param;"\
			"setenv bootargs ${bootargs} ${mtdbootparts}; "\
			"\0"\
        "cmdline_keys="\
			"setenv region_code US;"\
            "if keyman init 0x1234; then "\
                "if keyman read region_code ${loadaddr} str; then fi;"\
                "if keyman read deviceid ${loadaddr} str; then "\
                    "setenv bootargs ${bootargs} androidboot.deviceid=${deviceid};"\
                "fi;"\
            "fi;"\
			"kbi usid noprint;"\
			"if printenv usid; then "\
				"setenv bootargs ${bootargs} androidboot.serialno=${usid};"\
				"setenv serial ${usid}; setenv serial# ${usid};"\
			"else "\
				"setenv bootargs ${bootargs} androidboot.serialno=an400${cpu_id};"\
				"setenv serial an400${cpu_id}; setenv serial# an400${cpu_id};"\
			"fi;"\
			"kbi ethmac noprint;"\
			"setenv bootargs ${bootargs} mac=${eth_mac} androidboot.mac=${eth_mac};"\
            "setenv bootargs ${bootargs} androidboot.wificountrycode=${region_code};"\
	    "factory_provision init;"\
            "\0"\
        "upgrade_key="\
		"if gpio input GPIOD_2; then "\
            "echo detect upgrade key; run update;"\
            "fi;"\
            "\0"\
		"updateu="\
			"if tftp 1080000 u-boot.bin.signed; then "\
			"store boot_write bootloader 1080000 $filesize;"\
			"fi;"\
			"\0"\
		BOOTENV\
		"pxe_boot=dhcp; pxe get && pxe boot\0"\
		"bootcmd_storeboot=run storeboot\0"\
		"boot_targets=usb0 mmc0 mmc1 storeboot pxe dhcp\0"


#ifndef CONFIG_PXP_DDR
#define CONFIG_PREBOOT  \
            "run bcb_cmd; "\
            "run upgrade_check;"\
            "run init_display;"\
            "run storeargs;"\
            "run upgrade_key;" \
            "run switch_bootmode;"
#else
#define CONFIG_PREBOOT  "echo preboot"
#endif
/* #define CONFIG_ENV_IS_NOWHERE  1 */
#define CONFIG_ENV_SIZE   (64*1024)
#define CONFIG_FIT 1
#define CONFIG_OF_LIBFDT 1
#define CONFIG_ANDROID_BOOT_IMAGE 1
#define CONFIG_SYS_BOOTM_LEN (64<<20) /* Increase max gunzip size*/

/* ATTENTION */
/* DDR configs move to board/amlogic/[board]/firmware/timing.c */

/* running in sram */
//#define UBOOT_RUN_IN_SRAM
#ifdef UBOOT_RUN_IN_SRAM
#define CONFIG_SYS_INIT_SP_ADDR				(0x00200000)
/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN				(256*1024)
#else
#define CONFIG_SYS_INIT_SP_ADDR				(0x00200000)
#define CONFIG_SYS_MALLOC_LEN				(96*1024*1024)
#endif

//#define CONFIG_NR_DRAM_BANKS			1
/* ddr functions */
#define DDR_FULL_TEST            0 //0:disable, 1:enable. ddr full test
#define DDR_LOW_POWER            0 //0:disable, 1:enable. ddr clk gate for lp
#define DDR_ZQ_PD                0 //0:disable, 1:enable. ddr zq power down
#define DDR_USE_EXT_VREF         0 //0:disable, 1:enable. ddr use external vref
#define DDR4_TIMING_TEST         0 //0:disable, 1:enable. ddr4 timing test function
#define DDR_PLL_BYPASS           0 //0:disable, 1:enable. ddr pll bypass function

/* storage: emmc/nand/sd */
#define CONFIG_ENV_OVERWRITE
/* #define 	CONFIG_CMD_SAVEENV */
/* fixme, need fix*/

#if (defined(CONFIG_ENV_IS_IN_AMLNAND) || defined(CONFIG_ENV_IS_IN_MMC)) && defined(CONFIG_STORE_COMPATIBLE)
#error env in amlnand/mmc already be compatible;
#endif

/*
*				storage
*		|---------|---------|
*		|					|
*		emmc<--Compatible-->nand
*					|-------|-------|
*					|		|
*					MTD<-Exclusive->NFTL
*					|
*			|***************|***************|
*			slc-nand	SPI-nand	SPI-nor
*			(raw nand)
*/
/* axg only support slc nand */
/* swither for mtd nand which is for slc only. */


#if defined(CONFIG_AML_NAND) && defined(CONFIG_MESON_NFC)
#error CONFIG_AML_NAND/CONFIG_MESON_NFC can not support at the sametime;
#endif

#if (defined(CONFIG_AML_NAND) || defined(CONFIG_MESON_NFC)) && defined(CONFIG_MESON_FBOOT)
#error CONFIG_AML_NAND/CONFIG_MESON_NFC CONFIG _MESON_FBOOT can not support at the sametime;
#endif

#if defined(CONFIG_SPI_NAND) && defined(CONFIG_MTD_SPI_NAND) && defined(CONFIG_MESON_NFC)
#error CONFIG_SPI_NAND/CONFIG_MTD_SPI_NAND/CONFIG_MESON_NFC can not support at the sametime;
#endif

/* #define		CONFIG_AML_SD_EMMC 1 */
#ifdef		CONFIG_AML_SD_EMMC
	#define 	CONFIG_GENERIC_MMC 1
	#define 	CONFIG_CMD_MMC 1
	#define CONFIG_CMD_GPT 1
	#define	CONFIG_SYS_MMC_ENV_DEV 1
	#define CONFIG_EMMC_DDR52_EN 0
	#define CONFIG_EMMC_DDR52_CLK 35000000
#endif
#define		CONFIG_PARTITIONS 1
#if 0
#define 	CONFIG_SYS_NO_FLASH  1
#endif

#if defined CONFIG_MESON_NFC || defined CONFIG_SPI_NAND || defined CONFIG_MTD_SPI_NAND
	#define CONFIG_SYS_MAX_NAND_DEVICE  2
#endif

/* vpu */
#define AML_VPU_CLK_LEVEL_DFT 7

/* osd */
#define OSD_SCALE_ENABLE
#define AML_OSD_HIGH_VERSION

/* USB
 * Enable CONFIG_MUSB_HCD for Host functionalities MSC, keyboard
 * Enable CONFIG_MUSB_UDD for Device functionalities.
 */
/* #define CONFIG_MUSB_UDC		1 */
/* #define CONFIG_CMD_USB 1 */

#define USB_PHY2_PLL_PARAMETER_1	0x09400414
#define USB_PHY2_PLL_PARAMETER_2	0x927e0000
#define USB_PHY2_PLL_PARAMETER_3	0xAC5F49E5

#define USB_G12x_PHY_PLL_SETTING_1	(0xfe18)
#define USB_G12x_PHY_PLL_SETTING_2	(0xfff)
#define USB_G12x_PHY_PLL_SETTING_3	(0x78000)
#define USB_G12x_PHY_PLL_SETTING_4	(0xe0004)
#define USB_G12x_PHY_PLL_SETTING_5	(0xe000c)

#define AML_TXLX_USB        1
#define AML_USB_V2             1
#define USB_GENERAL_BIT         3
#define USB_PHY21_BIT           4

/* UBOOT fastboot config */


/* UBOOT Facotry usb/sdcard burning config */

/* net */
/* #define CONFIG_CMD_NET   1 */
#define CONFIG_ETH_DESIGNWARE
#if defined(CONFIG_CMD_NET)
	#define CONFIG_DESIGNWARE_ETH 1
	#define CONFIG_PHYLIB	1
	#define CONFIG_NET_MULTI 1
	#define CONFIG_CMD_PING 1
	#define CONFIG_CMD_DHCP 1
	#define CONFIG_CMD_RARP 1
	#define CONFIG_HOSTNAME        "arm_gxbb"
#if 0
	#define CONFIG_RANDOM_ETHADDR  1				   /* use random eth addr, or default */
#endif
	#define CONFIG_ETHADDR         00:15:18:01:81:31   /* Ethernet address */
	#define CONFIG_IPADDR          192.168.31.196          /* Our ip address */
	#define CONFIG_GATEWAYIP       192.168.31.1           /* Our getway ip address */
	#define CONFIG_SERVERIP        192.168.31.199        /* Tftp server ip address */
	#define CONFIG_NETMASK         255.255.255.0
#endif /* (CONFIG_CMD_NET) */

#define MAC_ADDR_NEW  1

/* other devices */
#define CONFIG_SHA1 1
#define CONFIG_MD5 1

/* commands */
/* #define CONFIG_CMD_PLLTEST 1 */

/*file system*/
#define CONFIG_DOS_PARTITION 1
#define CONFIG_EFI_PARTITION 1
/* #define CONFIG_MMC 1 */
#define CONFIG_FS_FAT 1
#define CONFIG_FS_EXT4 1
#define CONFIG_LZO 1

#define CONFIG_FAT_WRITE 1
#define CONFIG_AML_FACTORY_PROVISION 1

/* Cache Definitions */
/* #define CONFIG_SYS_DCACHE_OFF */
/* #define CONFIG_SYS_ICACHE_OFF */

/* other functions */
#define CONFIG_LIBAVB		1

/* define CONFIG_SYS_MEM_TOP_HIDE 8M space for free buffer */
#define CONFIG_SYS_MEM_TOP_HIDE		0x00800000

#define CONFIG_CPU_ARMV8

/* define CONFIG_UPDATE_MMU_TABLE for need update mmu */
#define CONFIG_UPDATE_MMU_TABLE

//use sha2 command
#define CONFIG_CMD_SHA2

//use hardware sha2
#define CONFIG_AML_HW_SHA2

//#define CONFIG_MULTI_DTB    1

/* support secure boot */
#define CONFIG_AML_SECURE_UBOOT   1

#if defined(CONFIG_AML_SECURE_UBOOT)

/* unify build for generate encrypted bootloader "u-boot.bin.encrypt" */
#define CONFIG_AML_CRYPTO_UBOOT   1
//#define CONFIG_AML_SIGNED_UBOOT   1
/* unify build for generate encrypted kernel image
   SRC : "board/amlogic/(board)/boot.img"
   DST : "fip/boot.img.encrypt" */
/* #define CONFIG_AML_CRYPTO_IMG       1 */

#endif /* CONFIG_AML_SECURE_UBOOT */

#define CONFIG_FIP_IMG_SUPPORT  1

#define BL32_SHARE_MEM_SIZE  0x800000

#endif

#undef CONFIG_SYS_CBSIZE
#define CONFIG_SYS_CBSIZE 1024

#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE 8192000
#define CONFIG_VIDEO_BMP_GZIP 1
