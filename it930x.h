/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * ITE IT930x driver
 *
 * Copyright (C) 2024 Xiaodong Ni <nxiaodong520@gmail.com>
 */

#ifndef IT930X_H
#define IT930X_H

#include <linux/platform_device.h>
#include <dvb_usb.h>
#include "avl6381.h"
#include "mxl603_tuner.h"

struct reg_val {
	u32 reg;
	u8  val;
};

struct reg_val_mask {
	u32 reg;
	u8  val;
	u8  mask;
};

struct usb_req {
	u8  cmd;
	u8  mbox;
	u8  wlen;
	u8  *wbuf;
	u8  rlen;
	u8  *rbuf;
};

struct state {
#define BUF_LEN 255
	u8 buf[BUF_LEN];
	u8 seq; /* packet sequence number */
	u8 prechip_version;
	u8 chip_version;
	u16 chip_type;
	u8 eeprom[256];
	bool no_eeprom;
	u8 ir_mode;
	u8 ir_type;
	u8 dual_mode:1;
};

/* USB commands */
#define CMD_MEM_RD                  0x00
#define CMD_MEM_WR                  0x01
#define CMD_I2C_RD                  0x02
#define CMD_I2C_WR                  0x03
#define CMD_IR_GET                  0x18
#define CMD_FW_DL                   0x21
#define CMD_FW_QUERYINFO            0x22
#define CMD_FW_BOOT                 0x23
#define CMD_FW_DL_BEGIN             0x24
#define CMD_FW_DL_END               0x25
#define CMD_FW_SCATTER_WR           0x29
#define CMD_GENERIC_I2C_RD          0x2a
#define CMD_GENERIC_I2C_WR          0x2b

enum it930x_gpio {
	IT930X_GPIO1,
	IT930X_GPIO2,
	IT930X_GPIO3,
	IT930X_GPIO4,
	IT930X_GPIO5,
	IT930X_GPIO6,
	IT930X_GPIO7,
	IT930X_GPIO8,
	IT930X_GPIO9,
	IT930X_GPIO10,
	IT930X_GPIO11,
	IT930X_GPIO12,
	IT930X_GPIO13,
	IT930X_GPIO14,
	IT930X_GPIO15,
	IT930X_GPIO16,
};

enum it930x_gpio_mode {
	IT930X_GPIO_IN,
	IT930X_GPIO_OUT,
};

enum it930x_gpio_enable {
	IT930X_GPIO_DISABLE,
	IT930X_GPIO_ENABLE,
};

enum it930x_gpio_level {
	IT930X_GPIO_LOW,
	IT930X_GPIO_HIGH,
};

//#define RUNQIDA_FIRMWARE_IT9303 "dvb-usb-it9303-runqida.fw"
#define ITE_FIRMWARE_IT9303 "dvb-usb-it9303-01.fw"

#endif
