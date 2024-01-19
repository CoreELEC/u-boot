#ifndef __AML_S905D3_CC_H__
#define __AML_S905D3_CC_H__

#define CONFIG_DEVICE_PRODUCT		"solitude"
#define ODROID_BOARD_UUID		"c21e5ce2-f4b8-4279-97b8-70147276f608"

/* configs for CEC */
#define CONFIG_CEC_OSD_NAME		"Solitude"
#define CONFIG_CEC_WAKEUP

#include "odroid-g12-common.h"

#undef ETHERNET_EXTERNAL_PHY

#endif
