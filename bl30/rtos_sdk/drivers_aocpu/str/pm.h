/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PM_H__
#define __PM_H__

#include "common.h"

#define WS_NEED_WAKEUP 1
#define WS_NEED_NOT_WAKEUP 0

#define listGET_OWNER_OF_PRE_ENTRY(pxTCB, pxList)                                                  \
	{                                                                                          \
		List_t *const pxConstList = (pxList);                                              \
		/* Increment the index to the next item and return the item, ensuring */           \
		/* we don't return the marker used at the end of the list.  */                     \
		if ((void *)(pxConstList)->pxIndex == (void *)&((pxConstList)->xListEnd)) {        \
			(pxConstList)->pxIndex = (pxConstList)->pxIndex->pxPrevious;               \
		}                                                                                  \
		(pxTCB) = (pxConstList)->pxIndex->pvOwner;                                         \
		(pxConstList)->pxIndex = (pxConstList)->pxIndex->pxPrevious;                       \
	}

struct platform_power_ops {
	int (*begin)(void);
	int (*end)(void);
};

struct dev_power_ops {
	int (*enter)(void *arg);
	int (*restore)(void *arg);
};

#if defined(SUPPORT_PM)
int pm_enter(void);
int pm_wake_up(void *arg);
int dev_unregiser_ws(void *arg);
int find_static_power_dev(void);
void wakeup_ap_from_kernel(void);
int set_platform_power_ops(struct platform_power_ops *ops);
struct wakeup_source *dev_register_ws(char *name, struct dev_power_ops *ops, void *data,
				      uint32_t need_wakeup_flag);
#else
static inline int pm_enter(void)
{
	return 0;
}

static inline void wakeup_ap_from_kernel(void)
{
	;
}
#endif
#endif
