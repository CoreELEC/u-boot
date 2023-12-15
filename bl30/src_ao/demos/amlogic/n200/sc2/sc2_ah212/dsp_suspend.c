/*
 * Copyright (C) 2014-2021 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "soc.h"
#include "dsp_suspend.h"
#include "suspend.h"
#include "mailbox-api.h"

static void *xMboxVadWakeup(void *msg)
{
	(void)msg;
	uint32_t buf[4] = {0};

	buf[0] = VAD_WAKEUP;
	STR_Wakeup_src_Queue_Send(buf);

	return NULL;
}

void vDSPVadWakeupInit(void)
{
	int ret;

	ret = xInstallRemoteMessageCallbackFeedBack(AODSPA_CHANNEL, MBX_CMD_VAD_AWE_WAKEUP,
									xMboxVadWakeup, 0);
	if (ret == MBOX_CALL_MAX)
		printf("mbox cmd 0x%x register fail\n", MBX_CMD_VAD_AWE_WAKEUP);
}

void vDSPVadWakeupDeinit(void)
{
	xUninstallRemoteMessageCallback(AODSPA_CHANNEL, MBX_CMD_VAD_AWE_WAKEUP);
}

/*use timerI to wakeup dsp FSM*/
static void wakeup_dsp(void)
{
	uint32_t value;

	/*set alarm timer*/
	REG32(DSP_FSM_TRIGER_SRC) = 10;/*10us*/

	value = REG32(DSP_FSM_TRIGER_CTRL);
	value &= ~((1 << 7) | (0x3) | (1 << 6));
	value |= ((1 << 7) | (0 << 6) | (0x3));
	REG32(DSP_FSM_TRIGER_CTRL) = value;
	vTaskDelay(2);
}

void vDSP_resume(uint32_t st_f)
{
	if ((!st_f) && (get_reason_flag() != VAD_WAKEUP))
		wakeup_dsp();
}

