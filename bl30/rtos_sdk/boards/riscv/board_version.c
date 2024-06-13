/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifdef CONFIG_BL30_VERSION_SAVE
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#endif
#include "board_version.h"
#include "sdk_ver.h"

void output_aocpu_info(void)
{
	printf("AOCPU image version='%s'\n", CONFIG_COMPILE_TIME);
	OUTPUT_VERSION_FULL_INFO();
}

#ifdef CONFIG_BL30_VERSION_SAVE
/* Applied to save bl30 version info to ddr with blx version info
 * when dump build message in uboot console.
 */
#include "mailbox-api.h"
#define GET_BL30_VERSION_SIZE 0x30303000
#define GET_BL30_VERSION_INFO 0x30303001
#define GET_BL30_VERSION_TIME 0x30303002
#define BL30_VERSION_LEN_ONCE_MAX MHU_DATA_SIZE

static uint8_t ver_items_num, ver_get_cur_items, ver_items_trans_once;
static int ver_total_size, ver_item_size;

static void *xMboxBL30VersionSave(void *msg)
{
	char *tmp_buf = msg;

	if (*(uint32_t *)msg == GET_BL30_VERSION_SIZE) {
		/* transfer version size */
		if (ver_items_trans_once == 0) {
			printf("bl30 version message once size error\n");
			*(uint32_t *)msg = 0;
			return NULL;
		}

		/* save version size that can be transferred once by mbox */
		ver_total_size |= (ver_items_trans_once * ver_item_size) << 16;
		*(uint32_t *)msg = ver_total_size;
	} else if (*(uint32_t *)msg == GET_BL30_VERSION_TIME) {
		/* transfer version compile time */
		if (strlen(CONFIG_COMPILE_TIME) <= BL30_VERSION_LEN_ONCE_MAX) {
			memset((void *)tmp_buf, 0, BL30_VERSION_LEN_ONCE_MAX);
			memcpy((void *)tmp_buf,
			       (void *)CONFIG_COMPILE_TIME,
			       strlen(CONFIG_COMPILE_TIME));
			tmp_buf += strlen(CONFIG_COMPILE_TIME);
			memcpy((void *)tmp_buf, (void *)"\n", 1);
		} else
			printf("bl30 version compile time size overflow\n");
	} else if (*(uint32_t *)msg == GET_BL30_VERSION_INFO) {
		/* transfer version info: version items */
		memset((void *)tmp_buf, 0, BL30_VERSION_LEN_ONCE_MAX);
		for (int i = 0; i < ver_items_trans_once; i++, ver_get_cur_items++) {
			if (ver_get_cur_items > ver_items_num)
				break;

			memcpy((void *)tmp_buf, (void *)"\t", 1);
			tmp_buf += 1;

			memcpy((void *)tmp_buf,
			       (void *)version_map[ver_get_cur_items],
			       strlen(version_map[ver_get_cur_items]));
			tmp_buf += strlen(version_map[ver_get_cur_items]);

			memcpy((void *)tmp_buf, "\n", 1);
			tmp_buf += 1;
		}
	} else {
		printf("bl30 version message get invalid msg\n");
		*(uint32_t *)msg = 0;
	}

	return NULL;
}

int bl30_plat_save_version(void)
{
	int ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AOREE_CHANNEL,
		MBX_CMD_SAVE_BL30_VERSION, xMboxBL30VersionSave, 1);
	if (ret == MBOX_CALL_MAX) {
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_SAVE_BL30_VERSION);
		return -ENOTSUP;
	}

	ver_items_num = sizeof(version_map) / sizeof(const char *);
	ver_item_size = strlen(version_map[0]) + 2;
	ver_items_trans_once = BL30_VERSION_LEN_ONCE_MAX / ver_item_size;
	ver_total_size = ver_items_num * ver_item_size;

	return 0;
}
#endif /* end CONFIG_BL30_VERSION_SAVE */
