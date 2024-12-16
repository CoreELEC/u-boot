/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "soc.h"
#include "dsp.h"
#include "suspend.h"
#include "mailbox-api.h"

extern uint32_t suspend_flag;
static void *xMboxVadWakeup(void *msg)
{
	*(uint32_t *)msg = suspend_flag;
	uint32_t buf[4] = {0};

	buf[0] = VAD_WAKEUP;
	STR_Wakeup_src_Queue_Send(buf);

	return NULL;
}

void vDSPVadWakeupInit(void)
{
	xInstallRemoteMessageCallbackFeedBack(AODSPA_CHANNEL,
					      MBX_CMD_VAD_AWE_WAKEUP,
					      xMboxVadWakeup,
					      1);
}

void vDSPVadWakeupDeinit(void)
{
	xUninstallRemoteMessageCallback(AODSPA_CHANNEL, MBX_CMD_VAD_AWE_WAKEUP);
}

