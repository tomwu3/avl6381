// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Availink avl6381 demod driver
 *
 * Copyright (C) 2024 Xiaodong Ni <nxiaodong520@gmail.com>
 */

#ifndef AVL6381_PRIV_H
#define AVL6381_PRIV_H

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
#include <dvb_frontend.h>
#else
#include <media/dvb_frontend.h>
#endif

//#include "avl6381_FwData_DTMB.h"
//#include "avl6381_FwData_DVBC.h"
//#include "avl6381_patch_dtmb.h"
//#include "avl6381_freezedata_fw.h"
#include "avl6381_freezeData_DTMB.h"

#define MAX_II2C_READ_SIZE  32
#define MAX_II2C_WRITE_SIZE 32

struct avl6381_priv {
	struct i2c_adapter *i2c;
 	struct avl6381_config *config;
	struct dvb_frontend frontend;
	enum fe_delivery_system delivery_system;
	int inited;
	struct mutex mutex;
	u16 g_nChannel_ts_total;
};

#endif

