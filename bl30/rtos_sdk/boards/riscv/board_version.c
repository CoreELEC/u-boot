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
#define BL30_VERSION_LEN_MAX 400
#ifdef BL2E_VER_BUFF_BASE_ADDR
#define TAG_NAME "BL30: "
#endif
int version_string(char *buf)
{
	char *tmp_buf;
	int size = 0;

	if (!buf) {
		printf("bl30 version string is NULL\n");
		return -EINVAL;
	}

	tmp_buf = buf;

	if (strlen(CONFIG_COMPILE_TIME)) {
#ifdef BL2E_VER_BUFF_BASE_ADDR
		size += strlen(TAG_NAME);
#endif
		size += strlen(CONFIG_COMPILE_TIME);
		size += 1;
		if (size > BL30_VERSION_LEN_MAX)
			return -ENOMEM;

#ifdef BL2E_VER_BUFF_BASE_ADDR
		memcpy((void *)tmp_buf, (void *)TAG_NAME, strlen(TAG_NAME));
		tmp_buf += strlen(TAG_NAME);
#endif
		memcpy((void *)tmp_buf, (void *)CONFIG_COMPILE_TIME, strlen(CONFIG_COMPILE_TIME));
		tmp_buf += strlen(CONFIG_COMPILE_TIME);

		memcpy((void *)tmp_buf, (void *)"\n", 1);
		tmp_buf += 1;
	}

	for (int i = 0; i < sizeof(version_map) / sizeof(const char *); i++) {
		size += 2;
		size += strlen(version_map[i]);
		if (size > BL30_VERSION_LEN_MAX)
			return -ENOMEM;

		memcpy((void *)tmp_buf, (void *)"\t", 1);
		tmp_buf += 1;

		memcpy((void *)tmp_buf, (void *)version_map[i], strlen(version_map[i]));
		tmp_buf += strlen(version_map[i]);

		memcpy((void *)tmp_buf, "\n", 1);
		tmp_buf += 1;
	}

	return size;
}

/*******************************************************************************
 * Save build message
 ******************************************************************************/
#ifdef BL2E_VER_BUFF_BASE_ADDR
int bl30_plat_save_version(void)
{
	struct build_messages_t *build_info;
	char rs[BL30_VERSION_LEN_MAX] = {0};
	int size;

	build_info = (struct build_messages_t *)(uintptr_t)(BL2E_VER_BUFF_BASE_ADDR);

	if (build_info->h.type != PARAM_MESSAGE ||
		build_info->h.version != VERSION_1 ||
		build_info->h.size != sizeof(struct build_messages_t)) {
		printf("invalid build messages info head!\n");
		return -EINVAL;
	}

	if ((sizeof(struct build_messages_t)
			+ build_info->bl2_message_size
			+ build_info->bl2e_message_size
			+ build_info->bl2x_message_size
			+ build_info->bl31_message_size
			+ build_info->bl32_message_size
			+ version_string(rs))
			> BL2E_VER_BUFF_SIZE) {
		printf("invalid bl30 build messages size!\n");
		return -EINVAL;
	}

	if (build_info->bl32_message_addr && build_info->bl32_message_size)
		build_info->bl30_message_addr = build_info->bl32_message_addr
				+ build_info->bl32_message_size;
	else if (build_info->bl31_message_addr && build_info->bl31_message_size)
		build_info->bl30_message_addr = build_info->bl31_message_addr
				+ build_info->bl31_message_size;
	else if (build_info->bl2x_message_addr && build_info->bl2x_message_size)
		build_info->bl30_message_addr = build_info->bl2x_message_addr
				+ build_info->bl2x_message_size;
	else if (build_info->bl2e_message_addr && build_info->bl2e_message_size)
		build_info->bl30_message_addr = build_info->bl2e_message_addr
				+ build_info->bl2e_message_size;
	else if (build_info->bl2_message_addr && build_info->bl2_message_size)
		build_info->bl30_message_addr = build_info->bl2_message_addr
				+ build_info->bl2_message_size;
	else
		build_info->bl30_message_addr = BL2E_VER_BUFF_BASE_ADDR
				+ sizeof(struct build_messages_t);

	build_info->bl30_message_size = version_string(rs);
#ifdef DEBUG
	printf("\n build_info->bl31_message_addr %d",
			(uint32_t)build_info->bl31_message_addr);
	printf("\n build_info->bl31_message_size %d",
			(uint32_t)build_info->bl31_message_size);
	printf("\n build_info->bl30_message_addr %d",
			(uint32_t)build_info->bl30_message_addr);
	printf("\n build_info->bl30_message_size %d",
			(uint32_t)build_info->bl30_message_size);
#endif
	if (build_info->bl30_message_size)
		memcpy((void *)(uintptr_t)build_info->bl30_message_addr, rs,
			build_info->bl30_message_size);
	return 0;
}
#else
/* Applied to save bl30 version info to ddr with blx version info
 * when dump build message in uboot console.
 */
#include "mailbox-api.h"
#define GET_BL30_VERSION_SIZE 0x30303000
#define GET_BL30_VERSION_INFO 0x30303001
#define BL30_VERSION_LEN_ONCE MHU_DATA_SIZE

static uint32_t ver_get_size;
static void *xMboxBL30VersionSave(void *msg)
{
	char vs[BL30_VERSION_LEN_MAX] = {0};
	int size;

	size = version_string(vs);
	if (size == -ENOMEM || size > BL30_VERSION_LEN_MAX ||
		ver_get_size >= BL30_VERSION_LEN_MAX) {
		printf("bl30 version message get fail: length overflow\n");
		*(uint32_t *)msg = 0;
		return NULL;
	}

	if (*(uint32_t *)msg == GET_BL30_VERSION_SIZE) {
		*(uint32_t *)msg = size;
		ver_get_size = 0;
	} else if (*(uint32_t *)msg == GET_BL30_VERSION_INFO) {
		memcpy(msg, &vs[ver_get_size], BL30_VERSION_LEN_ONCE);
		ver_get_size += BL30_VERSION_LEN_ONCE;
	} else {
		printf("bl30 version message get invalid msg\n");
		*(uint32_t *)msg = 0;
		return NULL;
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

	return 0;
}
#endif
#endif /* end CONFIG_BL30_VERSION_SAVE */
