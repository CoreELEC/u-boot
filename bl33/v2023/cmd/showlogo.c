/*
 * Copyright (C) 2025 Hardkernel Co,. Ltd
 * Dongjin Kim <tobetter@gmail.com>
 *
 *  This driver has been modified to support ODROID-C5.
 *      Modified by Bob Shin <bob.shin@hardkernel.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <vsprintf.h>

static int boot_partition(void)
{
	// TODO
	return 1;
}

static int init_hdmi(const char* mode, bool bestmode) {
	/*
	 * TODO reboot mode
	 * run_command("get_rebootmode", 0);
	 * rbm = env_get("reboot_mode");
	 * if rbm in {quiescent, recovery_quiescent} then pass hdmi init
	 */
	int ret;
	env_set("hdmimode", mode);

	if (bestmode == false) {
		env_set("is.bestmode", "false");
	}

	/* hdmi config */
	ret = run_command("hdmitx hpd", 0);
	if (ret < 0) {
		printf("hdmi not connected!\n");
		return -1;
	}

	ret = run_command("hdmitx get_parse_edid", 0);
	if (ret < 0) {
		printf("edid parse fail!\n");
		return -1;
	}
	return 0;
}

static int display_logo(const char* mode, const char* bmp_width, const char* bmp_height)
{
	int ret = 0;
	char str[64];
	int bootdev;
	int bmp_load = false;

	// TODO bestmode
	ret = init_hdmi(mode, true);
	if (ret) {
		printf("init hdmi fail!\n");
		return -1;
	}

	/* check boot device */
	bootdev = boot_partition();

	env_set("bootlogo_addr", env_get("loadaddr"));
#ifdef CONFIG_VIDEO_BMP_GZIP
	sprintf(str, "load mmc %d ${bootlogo_addr} boot-logo.bmp.gz", bootdev);
	ret = run_command(str, 0);
	if (!ret)	bmp_load = true;
#endif
	sprintf(str, "load mmc %d ${bootlogo_addr} boot-logo.bmp", bootdev);
	ret = run_command(str, 0);
	if (!ret)	bmp_load = true;


	if (bmp_load) {
		/* color */
		env_set("display_bpp", "24");
		env_set("display_color_index", "24");
		// env_set("display_color_fg", "0xffff");
		// env_set("display_color_bg", "0");
		// env_set("cvbsmode", "576cvbs");

		/* frame buffer */

		// option. fb size from disp size
		// env_set("fb_width", env_get("display_width"));
		// env_set("fb_height", env_get("display_height"));

		// else
		env_set("fb_width", bmp_width);
		env_set("fb_height", bmp_height);

		/* osd, bmp */
		run_command("osd open; osd clear", 0);
		run_command("bmp display ${bootlogo_addr}", 0);
		run_command("bmp scale", 0);
	}

	run_command("vout output $outputmode", 0);
	return 0;
}

static int do_showlogo(cmd_tbl_t *cmdtp, int flag, int argc,
		char *const argv[])
{
	char *mode;

	if (argc <= 1) {
		mode = env_get("hdmimode");
		/* ODROID default logo size 1280x720 */
		display_logo((NULL == mode ? "none" : mode), "1280", "720");
	} else if (argc == 4) {
		display_logo(argv[1], argv[2], argv[3]);
	} else {
		display_logo(argv[1], "1280", "720");
	}

	return 0;
}

U_BOOT_CMD(
		showlogo,		4,		0,	do_showlogo,
		"Displaying BMP logo file to HDMI screen with the specified resolution",
		"<resolution> [<bmp_width> <bmp_height>]\n"
		"	resolution - screen resoltuion on HDMI screen\n"
		"		'1080p60hz' will be used by default if missing\n"
		"	bmp_width (optional) - width of logo bmp file\n"
		"		'1280' will be used by default if missing\n"
		"	bmp_height (optional) - height of logo bmp file\n"
		"		'720' will be used by default if missing"
);
