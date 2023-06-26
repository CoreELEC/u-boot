/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PM_H__
#define __PM_H__

#include "common.h"

#ifndef MTAG
#define MTAG "dft"
#endif

#define _sys_log_(arg, ...)	printf(arg, ##__VA_ARGS__)

#define sys_log		_sys_log_

#define logi(arg, ...)	sys_log("[30/%s] "arg, \
		MTAG, ##__VA_ARGS__)

#define loge(arg, ...)	sys_log("[30/%s] [error]%s:%d "arg, \
		MTAG, __func__, __LINE__, ##__VA_ARGS__)

typedef int (*initcall_t) (void);
#define ____define_initcall(fn, id) \
	initcall_t __initcall_##id##_##fn = (void *)fn

#define __define_initcall(fn, id) ____define_initcall(fn, id)

#define device_initcall(fn) __define_initcall(fn, 6)

#define WS_NEED_WAKEUP		1
#define WS_NEED_NOT_WAKEUP	0

#define listGET_OWNER_OF_PRE_ENTRY(pxTCB, pxList)	\
{	\
	List_t * const pxConstList = (pxList);	\
		/* Increment the index to the next item and return the item, ensuring */	\
		/* we don't return the marker used at the end of the list.  */	\
		if ((void *)(pxConstList)->pxIndex == (void *)&((pxConstList)->xListEnd)) { \
			(pxConstList)->pxIndex = (pxConstList)->pxIndex->pxPrevious;	\
		}	\
		(pxTCB) = (pxConstList)->pxIndex->pvOwner;	\
		(pxConstList)->pxIndex = (pxConstList)->pxIndex->pxPrevious;	\
}

typedef void *ws_t;

struct platform_power_ops {
	int (*begin)(void);
	int (*end)(void);
};

struct dev_power_ops {
	int (*enter)(void *arg);
	int (*restore)(void *arg);
};

#if defined(CONFIG_PM)
int pm_enter(void);
int pm_wake_up(ws_t arg);
int dev_unregister_ws(ws_t arg);
int find_static_power_dev(void);
void wakeup_ap_from_kernel(void);
int set_platform_power_ops(struct platform_power_ops *ops);
struct wakeup_source *dev_register_ws(char *name, struct dev_power_ops *ops,
		void *data, uint32_t need_wakeup_flag);

extern uint32_t _module_pm_begin;
extern uint32_t _module_pm_end;
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
