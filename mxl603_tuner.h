// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * MaxLinear MXL603 tuner driver
 *
 * Copyright (C) 2024 Xiaodong Ni <nxiaodong520@gmail.com>
 */
 
#ifndef __MXL603_TUNER_H__
#define __MXL603_TUNER_H__

#include <linux/dvb/version.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
#include <dvb_frontend.h>
#else
#include <media/dvb_frontend.h>
#endif
#include "mxl603_api.h"

extern struct dvb_frontend *mxl603_attach(struct dvb_frontend *fe,
					    struct i2c_adapter *i2c, u8 addr,
					    struct mxl603_config *cfg);

#endif /* __MXL603_TUNER_H__ */
