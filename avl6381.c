// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Availink avl6381 demod driver
 *
 * Copyright (C) 2024 Xiaodong Ni <nxiaodong520@gmail.com>
 */

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/bitrev.h>
#include <linux/gpio.h>

#include "avl6381.h"
#include "avl6381_priv.h"

enum avl6381_mode {
	MODE_DTMB,
	MODE_DVBC
};

unsigned char AVL6381PLLConfig[][40] =
{
  {0x80, 0xC3, 0xC9, 0x01, 0x02, 0x32, 0x03, 0x05, 0x00, 0xA3, 0xE1, 0x11, 0x01, 0x22, 0x03, 0x0C, 0x19, 0x20, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x80, 0xFE, 0x21, 0x0A, 0x00, 0x1E, 0xDD, 0x04, 0x70, 0xBF, 0xCC, 0x03}, 
  {0x80, 0xC3, 0xC9, 0x01, 0x02, 0x2C, 0x03, 0x06, 0x00, 0xEF, 0x1C, 0x0D, 0x01, 0x22, 0x03, 0x0C, 0x19, 0x20, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x80, 0xFE, 0x21, 0x0A, 0x00, 0x1E, 0xDD, 0x04, 0x70, 0xBF, 0xCC, 0x03}, 
  {0x00, 0x24, 0xF4, 0x00, 0x01, 0x2A, 0x03, 0x05, 0x00, 0x90, 0x05, 0x10, 0x01, 0x36, 0x03, 0x0A, 0x18, 0x1B, 0x01, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x4C, 0x0A, 0x00, 0xA2, 0x4A, 0x04, 0x00, 0x90, 0xD0, 0x03}, 
  {0x00, 0x24, 0xF4, 0x00, 0x01, 0x2A, 0x03, 0x06, 0x00, 0xF8, 0x59, 0x0D, 0x01, 0x36, 0x03, 0x0A, 0x18, 0x1B, 0x01, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x4C, 0x0A, 0x00, 0xA2, 0x4A, 0x04, 0x00, 0x90, 0xD0, 0x03}, 
  {0x00, 0x36, 0x6E, 0x01, 0x02, 0x4B, 0x03, 0x06, 0x00, 0xA3, 0xE1, 0x11, 0x01, 0x23, 0x03, 0x0A, 0x15, 0x1C, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x03, 0x0A, 0x00, 0xB4, 0xC4, 0x04, 0x00, 0x87, 0x93, 0x03}, 
  {0x00, 0x36, 0x6E, 0x01, 0x01, 0x2A, 0x02, 0x09, 0x00, 0xF8, 0x59, 0x0D, 0x01, 0x2A, 0x02, 0x0C, 0x19, 0x02, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x03, 0x0A, 0x00, 0xB4, 0xC4, 0x04, 0x00, 0x87, 0x93, 0x03}, 
  {0xC0, 0xFC, 0x9B, 0x01, 0x03, 0x4A, 0x03, 0x05, 0x00, 0xF1, 0xE0, 0x0F, 0x01, 0x20, 0x03, 0x0A, 0x18, 0x1B, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x4C, 0x0A, 0x00, 0xA2, 0x4A, 0x04, 0x00, 0x90, 0xD0, 0x03}, 
  {0xC0, 0xFC, 0x9B, 0x01, 0x03, 0x4A, 0x03, 0x06, 0x80, 0x73, 0x3B, 0x0D, 0x01, 0x20, 0x03, 0x0A, 0x18, 0x1B, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x4C, 0x0A, 0x00, 0xA2, 0x4A, 0x04, 0x00, 0x90, 0xD0, 0x03}
};

unsigned char AVL6381SleepPLLConfig[][40] =
{
  {0x80, 0xC3, 0xC9, 0x01, 0x02, 0x2C, 0x03, 0x0C, 0x80, 0x77, 0x8E, 0x06, 0x01, 0x22, 0x03, 0x18, 0x20, 0x20, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x40, 0xFF, 0x10, 0x05, 0x70, 0xBF, 0xCC, 0x03, 0x70, 0xBF, 0xCC, 0x03}, 
  {0x00, 0x24, 0xF4, 0x00, 0x01, 0x2A, 0x03, 0x0C, 0x00, 0xFC, 0xAC, 0x06, 0x01, 0x36, 0x03, 0x14, 0x1B, 0x1B, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x26, 0x05, 0x00, 0x90, 0xD0, 0x03, 0x00, 0x90, 0xD0, 0x03}, 
  {0x00, 0x36, 0x6E, 0x01, 0x02, 0x37, 0x03, 0x0C, 0x80, 0x77, 0x8E, 0x06, 0x01, 0x23, 0x03, 0x14, 0x1C, 0x1C, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xBD, 0x01, 0x05, 0x00, 0x87, 0x93, 0x03, 0x00, 0x87, 0x93, 0x03}, 
  {0xC0, 0xFC, 0x9B, 0x01, 0x03, 0x4A, 0x03, 0x0C, 0xC0, 0xB9, 0x9D, 0x06, 0x01, 0x20, 0x03, 0x14, 0x1B, 0x1B, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x26, 0x05, 0x00, 0x90, 0xD0, 0x03, 0x00, 0x90, 0xD0, 0x03}
};

#define dbg_avl(fmt, args...) \
	do {\
		if (debug_avl)\
			printk("AVL: %s: " fmt "\n", __func__, ##args);\
	} while (0)
MODULE_PARM_DESC(debug_avl, "\n\t\t Enable AVL demodulator debug information");
static int debug_avl;
module_param(debug_avl, int, 0644);

static int avl6381_i2c_rd(struct avl6381_priv *priv, u8 *buf, int len)
{
	int ret;
	struct i2c_msg msg = {
			.addr= priv->config->demod_address,
			.flags= I2C_M_RD,
			.len  = len,
			.buf  = buf,
	};
	ret = i2c_transfer(priv->i2c, &msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		dev_warn(&priv->i2c->dev, "%s: i2c rd failed=%d " \
				"len=%d\n", KBUILD_MODNAME, ret, len);
		ret = -EREMOTEIO;
	}
	return ret;
}


static int avl6381_i2c_wr(struct avl6381_priv *priv, u8 *buf, int len)
{
	int ret;
	struct i2c_msg msg = {
		.addr= priv->config->demod_address,
		.flags = 0,
		.buf = buf,
		.len = len,
	};
	ret = i2c_transfer(priv->i2c, &msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		dev_warn(&priv->i2c->dev, "%s: i2c wr failed=%d " \
				"len=%d\n", KBUILD_MODNAME, ret, len);
		ret = -EREMOTEIO;
	}
	return ret;
}


static int avl6381_i2c_wrm(struct avl6381_priv *priv, u8 *buf, int len)
{
	int ret;
	struct i2c_msg msg = {
		.addr= priv->config->demod_address,
		.flags = 0,  //I2C_M_NOSTART,	/*i2c_transfer will emit a stop flag, so we should send 2 msg together,
						// * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/
		.buf = buf,
		.len = len,
	};
	ret = i2c_transfer(priv->i2c, &msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		dev_warn(&priv->i2c->dev, "%s: i2c wrm failed=%d " \
				"len=%d\n", KBUILD_MODNAME, ret, len);
		ret = -EREMOTEIO;
	}
	return ret;
}


/* write 32bit words at addr */
#define MAX_WORDS_WR_LEN	((MAX_II2C_WRITE_SIZE-3) / 4)
static int avl6381_i2c_wr_data(struct avl6381_priv *priv,
	u32 addr, u32 *data, int len)
{
	int ret = 0, this_len;
	u8 buf[MAX_II2C_WRITE_SIZE];
	u8 *p;

	while (len > 0) {
		p = buf;
		*(p++) = (u8) (addr >> 16);
		*(p++) = (u8) (addr >> 8);
		*(p++) = (u8) (addr);

		this_len = (len > MAX_WORDS_WR_LEN) ? MAX_WORDS_WR_LEN : len;
		len -= this_len;
		if (len)
			addr += this_len * 4;

		while (this_len--) {
			*(p++) = (u8) ((*data) >> 24);
			*(p++) = (u8) ((*data) >> 16);
			*(p++) = (u8) ((*data) >> 8);
			*(p++) = (u8) (*(data++));
		}

		if (len > 0)
			ret = avl6381_i2c_wrm(priv, buf, (int) (p - buf));
		else
			ret = avl6381_i2c_wr(priv, buf, (int) (p - buf));
		if (ret)
			break;
	}
	return ret;
}

static int avl6381_i2c_wr_reg(struct avl6381_priv *priv,
	u32 addr, u32 data, int reg_size)
{
	u8 buf[3 + 4];
	u8 *p = buf;

	*(p++) = (u8) (addr >> 16);
	*(p++) = (u8) (addr >> 8);
	*(p++) = (u8) (addr);

	switch (reg_size) {
	case 4:
		*(p++) = (u8) (data >> 24);
		*(p++) = (u8) (data >> 16);
	case 2:
		*(p++) = (u8) (data >> 8);
	case 1:
	default:
		*(p++) = (u8) (data);
		break;
	}

	return avl6381_i2c_wr(priv, buf, 3 + reg_size);
}

#define AVL6381_WR_REG8(_priv, _addr, _data) \
	avl6381_i2c_wr_reg(_priv, _addr, _data, 1)
#define AVL6381_WR_REG16(_priv, _addr, _data) \
	avl6381_i2c_wr_reg(_priv, _addr, _data, 2)
#define AVL6381_WR_REG32(_priv, _addr, _data) \
	avl6381_i2c_wr_reg(_priv, _addr, _data, 4)


static int avl6381_i2c_rd_reg(struct avl6381_priv *priv,
	u32 addr, u32 *data, int reg_size)
{
	int ret;
	u8 buf[3 + 4];
	u8 *p = buf;

	*(p++) = (u8) (addr >> 16);
	*(p++) = (u8) (addr >> 8);
	*(p++) = (u8) (addr);
	ret = avl6381_i2c_wr(priv, buf, 3);
	ret |= avl6381_i2c_rd(priv, buf, reg_size);

	*data = 0;
	p = buf;

	switch (reg_size) {
	case 4:
		*data |= (u32) (*(p++)) << 24;
		*data |= (u32) (*(p++)) << 16;
	case 2:
		*data |= (u32) (*(p++)) << 8;
	case 1:
	default:
		*data |= (u32) *(p);
		break;
	}
	return ret;
}

#define AVL6381_RD_REG8(_priv, _addr, _data) \
	avl6381_i2c_rd_reg(_priv, _addr, _data, 1)
#define AVL6381_RD_REG16(_priv, _addr, _data) \
	avl6381_i2c_rd_reg(_priv, _addr, _data, 2)
#define AVL6381_RD_REG32(_priv, _addr, _data) \
	avl6381_i2c_rd_reg(_priv, _addr, _data, 4)


static int GetRxOP_Status_6381(struct avl6381_priv *priv)
{
  int ret;
  int reg_data;

  reg_data = 0;
  ret = AVL6381_RD_REG32(priv, 0x000204, &reg_data);		//pj 0x000204 r3	win5842

  if ( !ret )
  {
    if ( reg_data )
      ret = 16LL;
  }
  return ret;
}

static int SendRxOP_6381(struct avl6381_priv *priv, int a2)
{
  int ret;
  signed int v7;

  v7 = 42;
  while ( ret = GetRxOP_Status_6381(priv) )
  {
    if ( !--v7 )
    {
      ret = 16;
      goto LABEL_11;
    }
    msleep(10);
  }
  ret |= AVL6381_WR_REG32(priv, 0x000204, (unsigned int)(a2 << 24));		//pj 0x000204 w 0x01000000	win5850

LABEL_11:
  return ret;
}

static int DigitalCoreReset_6381(struct avl6381_priv *priv)
{
  int ret;

  ret = AVL6381_WR_REG32(priv, 0x38FFFC, 0);
  msleep(10);
  return ret | AVL6381_WR_REG32(priv, 0x38FFFC, 1);
}

static int CheckChipReady_6381(struct avl6381_priv *priv)
{
  int ret;
  u32 v5 = 0;
  u32 v4 = 0;

	ret = AVL6381_RD_REG32(priv, 0x110840, &v5);					//pj 0x100840 r 4	win5762
	ret |= AVL6381_RD_REG32(priv, 0x0000A0, &v4);					//pj 0x0000A0 r 4
  if ( !ret )
  {
    if ( v5 == 1 )
    {
      ret = 2;
    }
    else if ( v4 != 1520795639 )
    {
      ret = 2;
    }
  }
  return ret;
}

static int AVL6381_GetFamilyID_Demod(struct avl6381_priv *priv, u32 *fid)
{
 return AVL6381_RD_REG32(priv, 0x040000, fid);
}

static int AVL6381_GetChipID(struct avl6381_priv *priv, u32 *chipid)
{
	int ret;
	u32 fid;
	
  ret = AVL6381_GetFamilyID_Demod(priv, &fid);
  dev_info(&priv->i2c->dev, "%s: ChipFamillyID:0x%x", KBUILD_MODNAME, fid);
  if ( fid == 0x63814e24 )
  {
		ret = AVL6381_RD_REG32(priv, 0x108004, chipid);
	  dev_info(&priv->i2c->dev, "%s: GetChipID:0x%x", KBUILD_MODNAME, chipid);
  }
  return ret;
}

static int DTMB_SetSpectrumPola_6381(struct avl6381_priv *priv, unsigned int a2)
{
	u8 data;
  int ret;

	if ( a2 == 1 )
	{
		ret |= AVL6381_WR_REG8(priv, 0x000322, 0x00);					//win5890
		ret |= AVL6381_WR_REG32(priv, 0x000324, 0x004C4B40); //check ??	//win5894
	}
	else
	{
		if ( a2 < 1 )
			data = 0x00;
		else
			data = 0x02;

		ret |= AVL6381_WR_REG8(priv, 0x000322, data);					//win5890
		ret |= AVL6381_WR_REG32(priv, 0x000324, 0xFFB3B4C0);	//win5894
	}
	
  return ret;
}

static int SetPLL_6381(struct avl6381_priv *priv, u8 *pll_conf)
{
	int ret;
	
	ret = AVL6381_WR_REG32(priv, 0x1000C0, pll_conf[4] - 1);//0x01);			//win634
	ret |= AVL6381_WR_REG32(priv, 0x1000C4, pll_conf[5] - 1);//0x4A);
	ret |= AVL6381_WR_REG32(priv, 0x1000D4, pll_conf[6]);//0x03);
	ret |= AVL6381_WR_REG32(priv, 0x1000C8, pll_conf[7] - 1);//0x05);
	ret |= AVL6381_WR_REG32(priv, 0x100080, pll_conf[12] - 1);//0x00);
	ret |= AVL6381_WR_REG32(priv, 0x100084, pll_conf[13] - 1);//0x22);
	ret |= AVL6381_WR_REG32(priv, 0x100094, pll_conf[14]);//0x03);
	ret |= AVL6381_WR_REG32(priv, 0x100088, pll_conf[15] - 1);//0x09);
//	msleep(5);
	ret |= AVL6381_WR_REG32(priv, 0x10008C, pll_conf[16] - 1);//0x14);
	ret |= AVL6381_WR_REG32(priv, 0x100090, pll_conf[17] - 1);//0x1B);
	ret |= AVL6381_WR_REG32(priv, 0x100000, 0x00);
	ret |= AVL6381_WR_REG32(priv, 0x100000, 0x01);
	msleep(5);
	ret |= AVL6381_WR_REG32(priv, 0x100018, pll_conf[5*4] | pll_conf[5*4+1]<<8 | pll_conf[5*4+2]<<16 | pll_conf[5*4+3]<<32);//0x6E);
	ret |= AVL6381_WR_REG32(priv, 0x10001C, pll_conf[6*4] | pll_conf[6*4+1]<<8 | pll_conf[6*4+2]<<16 | pll_conf[6*4+3]<<32);//0x0F);
	ret |= AVL6381_WR_REG32(priv, 0x100010, 0x01);
	ret |= AVL6381_WR_REG32(priv, 0x100008, 0x01);
	ret |= AVL6381_WR_REG32(priv, 0x100008, 0x00);		//win698
	
	return ret;
}

static int avl6381_wr_firmware(struct avl6381_priv *priv, u8 *data, int len)
{
	int ret;
	u8 buf[50];
	int size, pos;
	u32 addr;
	
	ret = 0;
	pos = 3;
	addr = (data[0]<<16) + (data[1]<<8) + data[2];
	while (pos < len)
	{
		if ((len - pos) >= 47)
			size = 50;
		else
			size = len - pos + 3;
		buf[0] = (addr>>16) & 0xFF;
		buf[1] = (addr>>8) & 0xFF;
		buf[2] = addr & 0xFF;
		memcpy(buf + 3, data + pos, size - 3);
		ret |= avl6381_i2c_wr(priv, buf, size);
		if (ret)
			break;
		pos += 47;
		addr += 47;
	}
	return ret;
}

static int avl6381_patch_new(struct avl6381_priv *priv)
{
  int i;
  int ret=0;
	u8 *v8;
	int v11;
	
	v8 = AVL6381_freezeData_DTMB;
  for ( i = 4; ; i += v11 + 8 )
  {
    v11 = (((((v8[i] << 8) + v8[i + 1]) << 8) + v8[i + 2]) << 8) + v8[i + 3];
    if ( !v11 | (i>sizeof(AVL6381_freezeData_DTMB)))
      break;
    ret |= avl6381_wr_firmware(priv, &v8[i + 5], v11 + 3);
  }
 
	ret |= AVL6381_WR_REG32(priv, 0x000228, 0x00280000);				//win5772
	ret |= AVL6381_WR_REG32(priv, 0x00022C, 0x002D0008);				//win5726
	ret |= AVL6381_WR_REG32(priv, 0x000230, 0x0028CB00);				//win5730
	ret |= AVL6381_WR_REG32(priv, 0x000234, 0x002F2C08);				//win5734
	ret |= AVL6381_WR_REG8(priv, 0x000225, 0x01);								//win5738
	ret |= AVL6381_WR_REG8(priv, 0x000226, 0x01);								//win5742		?
	ret |= AVL6381_WR_REG16(priv, 0x2D0000, 0x0001);						//win5746
	ret |= AVL6381_WR_REG16(priv, 0x2D0002, 0x0000);						//win5750
	ret |= AVL6381_WR_REG32(priv, 0x0000A0, 0x00000000);				//win5754
	ret |= AVL6381_WR_REG32(priv, 0x110840, 0x00000000);				//win5758

	return ret;
}

//static int avl6381_patch(struct avl6381_priv *priv, u8 patch_data[][], int data_size)
/*static int avl6381_patch(struct avl6381_priv *priv)
{
  int i;
  int ret=0;

	for (i=0; i<sizeof(avl6381_patch_freezedata)/51; i++)
	{
		ret |= avl6381_i2c_wr(priv, &avl6381_patch_freezedata[i][1], avl6381_patch_freezedata[i][0]);
//		ret |= avl6381_i2c_wr(priv, patch_data + i * data_size + 1, patch_data[i * data_size]);
	}

	ret |= AVL6381_WR_REG32(priv, 0x000228, 0x00280000);				//win5772
	ret |= AVL6381_WR_REG32(priv, 0x00022C, 0x002D0008);				//win5726
	ret |= AVL6381_WR_REG32(priv, 0x000230, 0x0028CB00);				//win5730
	ret |= AVL6381_WR_REG32(priv, 0x000234, 0x002F2C08);				//win5734
	ret |= AVL6381_WR_REG8(priv, 0x000225, 0x01);								//win5738
	ret |= AVL6381_WR_REG8(priv, 0x000226, 0x01);								//win5742		?
	ret |= AVL6381_WR_REG16(priv, 0x2D0000, 0x0001);						//win5746
	ret |= AVL6381_WR_REG16(priv, 0x2D0002, 0x0000);						//win5750
	ret |= AVL6381_WR_REG32(priv, 0x0000A0, 0x00000000);				//win5754
	ret |= AVL6381_WR_REG32(priv, 0x110840, 0x00000000);				//win5758

	return ret;
}*/

static int AVL6381_I2CBypassOn(struct avl6381_priv *priv)
{
	return AVL6381_WR_REG32(priv, 0x11801c, 0x00000007);
}

static int AVL6381_I2CBypassOff(struct avl6381_priv *priv)
{
	return AVL6381_WR_REG32(priv, 0x11801c, 0x00000006);
}

static int IBase_Initialize_6381(struct avl6381_priv *priv, u8 *pll_conf)
{
	int ret;

	ret = AVL6381_WR_REG32(priv, 0x110840, 0x00000001);				//win5758
	ret |= SetPLL_6381(priv, pll_conf);
  ret |= DigitalCoreReset_6381(priv);
  if (!ret)
		ret |= avl6381_patch_new(priv);
	
  return ret;
}

static int DVBC_InitRx_6381(struct avl6381_priv *priv)
{
	int ret;
	
  ret = SendRxOP_6381(priv, 1);
  if ( !ret )
  {
		ret |= AVL6381_WR_REG32(priv, 0x000560, 0x0d59f800);	//win5854
		ret |= AVL6381_WR_REG32(priv, 0x0005a8, 0x0a037a00);	//win5858
		ret |= AVL6381_WR_REG32(priv, 0x00055c, 0x016e3600);	//win5862
		ret |= AVL6381_WR_REG32(priv, 0x000580, 0x004c4b40);	//win5866
		ret |= AVL6381_WR_REG32(priv, 0x000558, 0x0068e778);	//win5866
  }
  return ret;
}

static int DVBC_InitADC_6381(struct avl6381_priv *priv)
{
  int ret;

	ret = AVL6381_WR_REG8(priv, 0x00057d, 0x01);					//win5870
	ret |= AVL6381_WR_REG8(priv, 0x00057f, 0x01);					//win5874
	ret |= AVL6381_WR_REG8(priv, 0x00057c, 0x00);					//win5878
	ret |= AVL6381_WR_REG8(priv, 0x000747, 0x00);					//win5882

  return ret;
}

static int DTMB_InitRx_6381(struct avl6381_priv *priv)
{
  int ret;

  ret = SendRxOP_6381(priv, 1);
  if ( !ret )
  {
		ret |= AVL6381_WR_REG32(priv, 0x000338, 0x11E1A300);	//win5854
		ret |= AVL6381_WR_REG32(priv, 0x000384, 0x0A037A00);	//win5858
		ret |= AVL6381_WR_REG32(priv, 0x00033C, 0x04C4B400);	//win5862
		ret |= AVL6381_WR_REG32(priv, 0x000304, 0x016E3600);	//win5866
		ret |= AVL6381_WR_REG8(priv, 0x000321, 0x01);					//win5870
		ret |= AVL6381_WR_REG8(priv, 0x000323, 0x01);					//win5874
		ret |= AVL6381_WR_REG8(priv, 0x000319, 0x00);					//win5878
		ret |= AVL6381_WR_REG8(priv, 0x00032B, 0x01);					//win5882
		ret |= AVL6381_WR_REG8(priv, 0x0000A6, 0x00);					//win5886
		ret |= DTMB_SetSpectrumPola_6381(priv, 1);	//check
  }
  
  return ret;
}

static int DTMB_InitADC_6381(struct avl6381_priv *priv)
{
  int ret;

//	ret = AVL6381_WR_REG8(priv, 0x000320, 0x01);
	ret = AVL6381_WR_REG8(priv, 0x000320, 0x00);	//check
//	ret |= AVL6381_WR_REG8(priv, 0x000324, 0x00);
	ret |= AVL6381_WR_REG8(priv, 0x0004d7, 0x00);	//check
	ret |= SendRxOP_6381(priv, 9);												//win5906
		
	return ret;
}

static int InitSDRAM_6381(struct avl6381_priv *priv)
{
  int ret;

	ret = AVL6381_WR_REG32(priv, 0x000210, 0x00070A00);	//win5918
	ret |= AVL6381_WR_REG32(priv, 0x000214, 0x05060600);	//win5922
	ret |= AVL6381_WR_REG32(priv, 0x000218, 0x03010301);	//win5926
	ret |= SendRxOP_6381(priv, 8LL);												//win5930

  return ret;
}

static int IRx_Initialize_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
	int ret;

	if (delivery_system == SYS_DVBC_ANNEX_A)
  {
    ret = DVBC_InitRx_6381(priv);
    ret |= DVBC_InitADC_6381(priv);
  }
  else
  {
    ret = DTMB_InitRx_6381(priv);
    ret |= DTMB_InitADC_6381(priv);
  }
  ret |= InitSDRAM_6381(priv);

  return ret;
}

static int DTMB_SetSymbolRate_6381(struct avl6381_priv *priv, unsigned int a2)
{
	return AVL6381_WR_REG32(priv, 0x000300, a2);			//win5942	w8 0x00735B40
}

static int DTMB_SetMpegMode_6381(struct avl6381_priv *priv)
{
  int ret;

	ret = AVL6381_WR_REG8(priv, 0x000352, 1);	//check ??					//win5946
	ret |= AVL6381_WR_REG8(priv, 0x000353, 1);						//win5950

  return ret;
}

static int DVBC_SetMpegMode_6381(struct avl6381_priv *priv)
{
  int ret;

	ret = AVL6381_WR_REG8(priv, 0x00056e, 0);						//win5946
	ret |= AVL6381_WR_REG8(priv, 0x00056f, 1);						//win5950

  return ret;
}

static int SetMpegMode_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  int ret;

	ret = 0;
	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
    ret = DTMB_SetMpegMode_6381(priv);
    break;
	case SYS_DVBC_ANNEX_A:
    ret = DVBC_SetMpegMode_6381(priv);
    break;
  }
  return ret;
}

static int SetMpegSerialPin_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  int ret;

	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = AVL6381_WR_REG8(priv, 0x000351, 0);						//win5954
    break;
	case SYS_DVBC_ANNEX_A:
		ret = AVL6381_WR_REG8(priv, 0x00056d, 0);						//win5954
    break;
  }

  return ret;
}

static int SetMpegSerialOrder_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  int ret;

	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = AVL6381_WR_REG8(priv, 0x000350, 0);						//win5958
    break;
	case SYS_DVBC_ANNEX_A:
		ret = AVL6381_WR_REG8(priv, 0x00056c, 0);						//win5958
    break;
  }
  
  return ret;
}

static int SetMpegSerialSyncPulse_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  int ret;

	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = AVL6381_WR_REG8(priv, 0x0004E6, 0);						//win5962
    break;
	case SYS_DVBC_ANNEX_A:
		ret = AVL6381_WR_REG8(priv, 0x00074e, 0);						//win5962
    break;
  }
  
  return ret;
}

static int SetMpegErrorBit_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  int ret;

	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = AVL6381_WR_REG8(priv, 0x000378, 1);						//win5966
    break;
	case SYS_DVBC_ANNEX_A:
		ret = AVL6381_WR_REG8(priv, 0x000578, 1);						//win5966
    break;
  }
  
  return ret;
}

static int SetMpegErrorPola_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  int ret;

	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = AVL6381_WR_REG8(priv, 0x000354, 0);						//win5970
    break;
	case SYS_DVBC_ANNEX_A:
		ret = AVL6381_WR_REG8(priv, 0x000570, 0);						//win5970
    break;
  }
  
  return ret;
}

static int SetMpegValidPola_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  int ret;

	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = AVL6381_WR_REG8(priv, 0x0004E7, 0);						//win5974
    break;
	case SYS_DVBC_ANNEX_A:
		ret = AVL6381_WR_REG8(priv, 0x00074f, 0);						//win5974
    break;
  }
  
  return ret;
}

static int SetMpegPacketLen_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  int ret;

	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = AVL6381_WR_REG8(priv, 0x000357, 0);						//win5978
    break;
	case SYS_DVBC_ANNEX_A:
		ret = AVL6381_WR_REG8(priv, 0x000573, 0);						//win5978
    break;
  }
  
  return ret;
}

static int DTMB_DisableMpegContinuousMode_6381(struct avl6381_priv *priv)
{
	return AVL6381_WR_REG8(priv, 0x00038B, 0);						//win5982
}

static int EnableMpegOutput_6381(struct avl6381_priv *priv)
{
	return AVL6381_WR_REG32(priv, 0x108030, 0x00000FFF);				//win5986 w32 0x00000FFF	
}

static int TunerI2C_Initialize_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  int ret;
  u32 reg;
  
 	ret = AVL6381_WR_REG32(priv, 0x118000, 0x01);												//win5990
	// close gate
	ret |= AVL6381_I2CBypassOff(priv);
//	ret |= AVL6381_WR_REG32(priv, 0x11801C, 0x6);												//win5994
	
	ret |= AVL6381_RD_REG32(priv, 0x118004, &reg);											//win5998
	reg &= 0xfffffffe;
	ret |= AVL6381_WR_REG32(priv, 0x118004, reg);											//win6006

	// set bit clock
//	ret |= AVL6381_WR_REG32(priv, 0x118018, I2C_RPT_DIV);		//win6010		data? 2B 1E
	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = AVL6381_WR_REG32(priv, 0x118018, 0x34);		//win6010		data? 2B 1E
    break;
	case SYS_DVBC_ANNEX_A:
		ret = AVL6381_WR_REG32(priv, 0x118018, 0x27);		//win6010		data? 2B 1E
    break;
	}
	
	// release from reset
	ret |= AVL6381_WR_REG32(priv, 0x118000, 0);													//win6014

  return ret;
}

static int SetAGCPola_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
	int ret;
	
	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = AVL6381_WR_REG8(priv, 0x00030B, 0);																									//win6018
    break;
	case SYS_DVBC_ANNEX_A:
		ret = AVL6381_WR_REG8(priv, 0x00059f, 0);																									//win6018   ?
    break;
	}
	
  return ret;
}

static int EnableAGC_6381(struct avl6381_priv *priv)
{
	return AVL6381_WR_REG32(priv, 0x108034, 0x00000001);																				//win6022
}

static int ResetPER_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
	int ret;
	u32 uiTemp = 0;

	ret = AVL6381_RD_REG32(priv, 0x149104, &uiTemp);							//win6070
	uiTemp |= 0x00000001;
	ret |= AVL6381_WR_REG32(priv, 0x149104, uiTemp);							//win6078

	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret |= AVL6381_WR_REG8(priv, 0x0000A5, 0x00);									//win6082
    break;
	case SYS_DVBC_ANNEX_A:
		ret |= AVL6381_WR_REG16(priv, 0x0001a2, 0x0000);				//check
    break;
	}
	
	ret |= AVL6381_RD_REG32(priv, 0x149104, &uiTemp);							//win6086
	uiTemp |= 0x00000008;
	ret |= AVL6381_WR_REG32(priv, 0x149104, uiTemp);							//win6094
	uiTemp |= 0x00000001;
	ret |= AVL6381_WR_REG32(priv, 0x149104, uiTemp);							//win6098
	uiTemp &= 0xFFFFFFFE;
	ret |= AVL6381_WR_REG32(priv, 0x149104, uiTemp);							//win6102

	return ret;
}

static int ResetErrorStat_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
	u32 reg;
	int ret;

	ret = AVL6381_RD_REG32(priv, 0x149160, &reg);											//win5998
  if ( reg == 1 )
  {
		ret |= AVL6381_WR_REG32(priv, 0x149128, 0);									//win6058
		ret |= AVL6381_WR_REG32(priv, 0x149128, 1);									//win6062
		ret |= AVL6381_WR_REG32(priv, 0x149128, 0);									//win6066
  }
  return ret | ResetPER_6381(priv, delivery_system);
}

static int InitErrorStat_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
	int ret;
	
 	ret = AVL6381_WR_REG32(priv,	0x149160, 0x00000001);					//win6026
	ret |= AVL6381_WR_REG32(priv, 0x14912C, 0x00000001);					//win6030
	ret |= AVL6381_WR_REG32(priv, 0x149130, 0x0A037A00);					//win6034
	ret |= AVL6381_WR_REG32(priv, 0x149134, 0x00000000);					//win6038
	ret |= AVL6381_WR_REG32(priv, 0x149138, 0x00000000);					//win6042
	ret |= AVL6381_WR_REG32(priv, 0x14913C, 0x00000000);					//win6046

  ret |= ResetErrorStat_6381(priv, delivery_system);

  return ret;
}

static int AVL6381_Initialize(struct avl6381_priv *priv)
{
	u32 chipid;
	int tryno, ret;

	if ( !priv->inited && !AVL6381_GetChipID(priv, &chipid) )
  {
  	priv->delivery_system = SYS_DVBC_ANNEX_A;
		ret = IBase_Initialize_6381(priv, AVL6381PLLConfig[5]);
    if ( !ret )
    {
      msleep(20);
      tryno = 20;
      while ( tryno && CheckChipReady_6381(priv))
      {
        msleep(20);
        tryno--;
      }
      ret |= IRx_Initialize_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= DTMB_SetSymbolRate_6381(priv, 7560000);	//check
      ret |= SetMpegMode_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= SetMpegSerialPin_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= SetMpegSerialOrder_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= SetMpegSerialSyncPulse_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= SetMpegErrorBit_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= SetMpegErrorPola_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= SetMpegValidPola_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= SetMpegPacketLen_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= DTMB_DisableMpegContinuousMode_6381(priv);
      ret |= EnableMpegOutput_6381(priv);
      ret |= TunerI2C_Initialize_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= SetAGCPola_6381(priv, SYS_DVBC_ANNEX_A);
      ret |= EnableAGC_6381(priv);
      ret |= InitErrorStat_6381(priv, SYS_DVBC_ANNEX_A);
			ret |= AVL6381_WR_REG32(priv, 0x0006F4, 0x0000000A);																				//win6106
			if (!ret)
				priv->inited = 1;
		}
  }
  return ret;
}

static int DTMB_GetLockStatus_6381(struct avl6381_priv *priv, u32 *status)
{
  int ret;
  u32 data;
  
	ret = AVL6381_RD_REG8(priv, 0x0000a6, &data);
	if (!ret)
		*status = data;
		
	return ret;
}

static int DVBC_GetLockStatus_6381(struct avl6381_priv *priv, u32 *status)
{
	u32 data;
  int ret;

	*status = 0;
	ret = AVL6381_RD_REG32(priv, 0x0001a4, &data);
	if (!ret && data==21)
		*status = 1;
	
  return ret;
}

static int AVL6381_GetLockStatus(struct avl6381_priv *priv, u32 *status)
{
	int ret=0;

	switch (priv->delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret |= DTMB_GetLockStatus_6381(priv, status);
    break;
	case SYS_DVBC_ANNEX_A:
		ret |= DVBC_GetLockStatus_6381(priv, status);
    break;
	}
	
  return ret;
}

static int DTMB_GetSNR_6381(struct avl6381_priv *priv, u32 *snr)
{
	return AVL6381_RD_REG16(priv, 0x00011c, snr);	//check
}

static int DVBC_GetSNR_6381(struct avl6381_priv *priv, u32 *snr)
{
	int v9, ret;
	
  ret = AVL6381_RD_REG32(priv, 0x0005d8, &v9);
  if ( !ret && !v9 )
  {
    ret |= AVL6381_RD_REG16(priv, 0x0001ae, snr);
		ret |= AVL6381_WR_REG32(priv, 0x0005d8, 0x00000001);
    msleep(50);
  }
  
  return ret;
}

static int AVL6381_GetSNR(struct avl6381_priv *priv, u32 *snr)
{
	int ret;
	
	switch (priv->delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = DTMB_GetSNR_6381(priv, snr);
    break;
	case SYS_DVBC_ANNEX_A:
		ret = DVBC_GetSNR_6381(priv, snr);
    break;
	}
	    
  return ret;
}

static s64 AVL6381QAMGetSNR(struct avl6381_priv *priv)
{
  s64 ret;
  u32 v2;

  v2 = 0;
  if ( !AVL6381_GetSNR(priv, &v2) )
    ret = v2;
  return ret;
}

static int DVBC_Halt_6381(struct avl6381_priv *priv)
{
	u32 v6;
	int ret;
	
  ret = AVL6381_RD_REG32(priv, 0x0001a0, &v6);
  v6 &= 0xFEu;
  ret |= AVL6381_WR_REG32(priv, 0x0001a0, v6);
  return ret;
}

static int DTMB_Halt_6381(struct avl6381_priv *priv)
{
	u32 v6;
	int ret;
	
  ret = AVL6381_RD_REG32(priv, 0x0000a4, &v6);
  v6 &= 0xFE;
  ret |= AVL6381_WR_REG32(priv, 0x0000a4, v6);
  return ret;
}

static int Halt_6381(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
	int ret;

  ret = SendRxOP_6381(priv, 3);
  msleep(2);
	switch (delivery_system) {  
	case SYS_DVBC_ANNEX_A:
		ret = DVBC_Halt_6381(priv);
    break;
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = DTMB_Halt_6381(priv);
    break;
	}
	
  return ret;
}

static int DTMB_GetRunningLevel_6381(struct avl6381_priv *priv, u32 *level)
{
  int ret;
  u32 v5;

	ret = AVL6381_RD_REG8(priv, 0x000124, &v5);
	if (!ret && v5)
    *level = 2;
  else
    *level = 0;
		
  return ret;
}

static int DVBC_GetRunningLevel_6381(struct avl6381_priv *priv, u32 *level)
{
  int ret;
  u32 v5;

	ret = AVL6381_RD_REG32(priv, 0x0001a4, &v5);
	if (v5)
    *level = 2;
  else
    *level = 0;
		
  return ret;
}

static int GetRunningLevel_6381(struct avl6381_priv *priv, u32 *level)
{
	int ret;

	switch (priv->delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = DTMB_GetRunningLevel_6381(priv, level);
    break;
	case SYS_DVBC_ANNEX_A:
		ret = DVBC_GetRunningLevel_6381(priv, level);
    break;
	}
	
  return ret;
}

static int DTMB_AutoLockChannel_6381(struct avl6381_priv *priv)
{
	int ret;
	
  ret = AVL6381_WR_REG8(priv, 0x00021f, 0x01);
  ret |= AVL6381_WR_REG8(priv, 0x0000ad, 0x00);
  ret |= AVL6381_WR_REG32(priv, 0x00010c, 0x00000000);
  ret |= SendRxOP_6381(priv, 2);
  return ret;
}

static int AutoLockChannel_6381(struct avl6381_priv *priv)
{
	int ret;

	switch (priv->delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
    ret = DTMB_AutoLockChannel_6381(priv);
    break;
	case SYS_DVBC_ANNEX_A:
    ret = SendRxOP_6381(priv, 12);
    break;
	}
	
  return ret;
}

static int AVL6381_AutoLock(struct avl6381_priv *priv, enum fe_delivery_system delivery_system)
{
  unsigned int v4;
  int v7;
  u32 v9;
  int ret;

	v9 = 2;	
  if ( !Halt_6381(priv, delivery_system) )
  {
    v4 = 0;
    while ( v9 && v4 <= 9 )
    {
      ret = GetRunningLevel_6381(priv, &v9);
      if ( ret )
      {
        return ret;
      }
      ++v4;
      msleep(10);
    }
    if ( v9 )
    {
      ret = 16;
    }
    else
    {
      ret = AutoLockChannel_6381(priv);
      if ( ret )
      {
      }
      else
      {
        v7 = 0;
        while ( v9 && v7 != 10 )
        {
          ret = GetRunningLevel_6381(priv, &v9);
          if ( ret )
          {
            return ret;
          }
          ++v7;
          msleep(10);
        }
      }
    }
  }
  return ret;
}

static int DTMB_NoSignalDetection_6381(struct avl6381_priv *priv, u32 *a2)
{
  int ret;
  u32 v5;

  *a2 = 0;
  v5 = 0;
  ret = AVL6381_RD_REG16(priv, 0x000146, &v5);
  if ( v5 > 3u )
    *a2 = 1;
    
  return ret;
}

static int DVBC_NoSignalDetection_6381(struct avl6381_priv *priv, u32 *a2)
{
	return AVL6381_RD_REG32(priv, 0x0001b8, a2);
}

static int AVL6381_NoSignalDetection(struct avl6381_priv *priv, u32 *a2)
{
  int ret;

	switch (priv->delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
    ret = DTMB_NoSignalDetection_6381(priv, a2);
    break;
	case SYS_DVBC_ANNEX_A:
    ret = DVBC_NoSignalDetection_6381(priv, a2);
    break;
	}
	
  return ret;
}

static int GetMode_6381(struct avl6381_priv *priv, u32 *mode)
{
	int v3, ret;
	
  ret = AVL6381_RD_REG32(priv, 0x000200, &v3);
  if ( !ret )
  	*mode = v3;
  
  return ret;
}

static int AVL6381_SetMode(struct avl6381_priv *priv, enum avl6381_mode mode)
{
  int rep, ret;
  u32 ds;
  enum fe_delivery_system delivery_system;

  ret = GetMode_6381(priv, &ds);
  if ( ds != mode )
  {
      rep = 22;

			if (mode==MODE_DTMB)
				delivery_system = SYS_DVBT2;
			else
				delivery_system = SYS_DVBC_ANNEX_A;
      	ret |= Halt_6381(priv, delivery_system);
      if ( !ret )
      {
        while ( GetRxOP_Status_6381(priv) )
        {
          if ( !--rep )
            return 16LL;
          msleep(20);
        }
      }
      ret |= AVL6381_WR_REG32(priv, 0x110084, 0);
      msleep(10);
      ret |= AVL6381_WR_REG32(priv, 0x110084, 1);
      ret |= AVL6381_WR_REG32(priv, 0x0000a0, 0);
      if ( !(SendRxOP_6381(priv, 10) | ret) )
      {
        rep = 202;
        while ( GetRxOP_Status_6381(priv) )
        {
          if ( !--rep )
            return 16LL;
          msleep(20);
        }
      }
      rep = 22;
      while ( CheckChipReady_6381(priv) )
      {
        if ( !--rep )
          return 16LL;
        msleep(20);
      }
			ret |= AVL6381_WR_REG32(priv, 0x110840, 1);
			
			switch (delivery_system) {
				case SYS_DVBT:
				case SYS_DVBT2:
		      ret |= SetPLL_6381(priv, AVL6381PLLConfig[4]);
		      break;
				case SYS_DVBC_ANNEX_A:
				case SYS_DVBC_ANNEX_B:
		      ret |= SetPLL_6381(priv, AVL6381PLLConfig[5]);
		      break;
			}
//      msleep(50);
//			ret |= AVL6381_WR_REG32(priv, 0x0000a0, 0);	//check
      msleep(20);
			ret |= AVL6381_WR_REG32(priv, 0x110840, 0);

    rep = 200;
    msleep(20);
    while ( CheckChipReady_6381(priv) )
    {
      msleep(20);
      if ( !--rep )
        return 16LL;
    }

    ret |= IRx_Initialize_6381(priv, delivery_system);
    if ( delivery_system == SYS_DVBT | delivery_system == SYS_DVBT2 ) //check
      ret |= DTMB_SetSymbolRate_6381(priv, 7560000);
    ret |= SetMpegMode_6381(priv, delivery_system);
    ret |= SetMpegSerialPin_6381(priv, delivery_system);
    ret |= SetMpegSerialOrder_6381(priv, delivery_system);
    ret |= SetMpegSerialSyncPulse_6381(priv, delivery_system);
    ret |= SetMpegErrorBit_6381(priv, delivery_system);
    ret |= SetMpegErrorPola_6381(priv, delivery_system);
    ret |= SetMpegValidPola_6381(priv, delivery_system);
    ret |= SetMpegPacketLen_6381(priv, delivery_system);
    ret |= DTMB_DisableMpegContinuousMode_6381(priv);
    ret |= EnableMpegOutput_6381(priv);
    ret |= TunerI2C_Initialize_6381(priv, delivery_system);
    ret |= SetAGCPola_6381(priv, delivery_system);
    ret |= EnableAGC_6381(priv);
    ret |= InitErrorStat_6381(priv, delivery_system);
    ret |= AVL6381_WR_REG32(priv, 0x0006F4, 0x0000000A);																				//win6106
    
  }
  return ret;
}

/*static int SetSleepClock_6381(struct avl6381_priv *priv)
{
	int rep, ret;
	
  ret = AVL6381_WR_REG32(priv, 0x110840, 1);
	ret |= SetPLL_6381(priv, AVL6381SleepPLLConfig[0]);
  ret |= AVL6381_WR_REG32(priv, 0x110840, 0);
  rep = 200;
  msleep(20);
  while ( CheckChipReady_6381(priv) )
  {
    msleep(20);
    if ( !--rep )
      return 16LL;
  }

  return ret;
}

static int AVL6381_Sleep(struct avl6381_priv *priv)
{
  int v1, ret;

  v1 = 10;
  ret = SetSleepClock_6381(priv);
  ret |= TunerI2C_Initialize_6381(priv, priv->delivery_system);
  ret |= SendRxOP_6381(priv, 6);
  do
  {
    ret |= GetRxOP_Status_6381(priv);
    if ( !ret )
      break;
    msleep(10);
    --v1;
  } while ( v1 );

  ret |= AVL6381_WR_REG32(priv, 0x110084, 1);
  
  return ret;
}*/

static int avl6381_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct avl6381_priv *priv = fe->demodulator_priv;
	int ret;

	dev_dbg(&priv->i2c->dev, "%s: %d\n", __func__, enable);

	if (enable) {
		ret = AVL6381_I2CBypassOn(priv);
		ret |= AVL6381_I2CBypassOn(priv);
		ret |= AVL6381_I2CBypassOn(priv);
		ret |= AVL6381_I2CBypassOn(priv);
		ret |= AVL6381_I2CBypassOn(priv);
	}
	else
	{
		ret = AVL6381_I2CBypassOff(priv);
		ret |= AVL6381_I2CBypassOff(priv);
		ret |= AVL6381_I2CBypassOff(priv);
		ret |= AVL6381_I2CBypassOff(priv);
		ret |= AVL6381_I2CBypassOff(priv);
	}
	
	return ret;
}


//#define I2C_RPT_DIV ((0x2A)*(250000)/(240*1000))	//m_CoreFrequency_Hz 250000000

static int avl6381_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	struct avl6381_priv *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret = 0;
	u32 st=0, reg1, reg2;
	s64 snr = 0;

	mutex_lock(&priv->mutex);

	*status  = 0;
	u16 strength = 0;
	int v5 = 29 ;
	while (v5>0 && !AVL6381_GetLockStatus(priv, &st))
	{
		if (st)
		{
			strength = 0;
			if (fe->ops.tuner_ops.get_rf_strength && fe->ops.tuner_ops.get_rf_strength(fe, &strength))
			{
				return 0;
			}
			if (strength)
			{
				*status |= FE_HAS_SYNC | FE_HAS_LOCK;
				break;
			}
		}
		msleep(20);
		--v5;
	}

	if (status)
	{
		snr = AVL6381QAMGetSNR(priv);

		AVL6381_RD_REG32(priv, 0x149110, &reg1);
		AVL6381_RD_REG32(priv, 0x149114, &reg2);
	//	AVL6381_RD_REG8(priv, 0x0000a5, &reg);

		c->strength.stat[0].scale = FE_SCALE_DECIBEL;
		c->strength.stat[0].svalue = (~strength + 1)*10;

		c->cnr.stat[0].scale = FE_SCALE_DECIBEL;
		c->cnr.stat[0].svalue = snr * 10;
		
		c->pre_bit_error.stat[0].scale = FE_SCALE_COUNTER;
		c->pre_bit_error.stat[0].uvalue = reg1;
		c->pre_bit_count.stat[0].scale = FE_SCALE_COUNTER;
		c->pre_bit_count.stat[0].uvalue = reg2;

	//	c->block_error.stat[0].scale = FE_SCALE_COUNTER;
	//	c->block_error.stat[0].uvalue = reg1;
	//	c->block_count.stat[0].scale = FE_SCALE_COUNTER;
	//	c->block_error.stat[0].uvalue = reg2;

	//dev_info(&priv->i2c->dev, "st:%d snr:%d strength:%d status:%x, reg1:%d, reg2:%d", st, snr, strength, *status, reg1, reg2);
	}
	mutex_unlock(&priv->mutex);

	return ret;
}

static enum dvbfe_algo avl6862fe_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}
			

static int avl6381_set_frontend(struct dvb_frontend *fe)
{
	struct avl6381_priv *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	u32 demod_mode;
	int ret = 0;
	u32 v29;

	mutex_lock(&priv->mutex);

	/* setup tuner */
	if (priv->config->tuner_select_input)
		ret |= priv->config->tuner_select_input(fe, c->delivery_system);

	switch (c->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBT2:
		ret |= AVL6381_SetMode(priv, MODE_DTMB);
		break;
	case SYS_DVBC_ANNEX_A:
		ret |= AVL6381_SetMode(priv, MODE_DVBC);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (fe->ops.tuner_ops.set_params)
		ret |= fe->ops.tuner_ops.set_params(fe);

	if (c->delivery_system==SYS_DVBT | c->delivery_system==SYS_DVBT2)
  	ret |= DTMB_SetSymbolRate_6381(priv, 7560000);

	ret |= AVL6381_AutoLock(priv, c->delivery_system);
	
	if (!ret)
		priv->delivery_system = c->delivery_system;
		
	mutex_unlock(&priv->mutex);
	
	return ret;
}

static int avl6381_init(struct dvb_frontend *fe)
{
	struct avl6381_priv *priv = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret=0;
	
	mutex_init(&priv->mutex);

	c->strength.len = 1;
	c->strength.stat[0].scale = FE_SCALE_DECIBEL;
	c->cnr.len = 1;
	c->cnr.stat[0].scale = FE_SCALE_DECIBEL;
	c->block_error.len = 1;
	c->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	c->pre_bit_error.len = 1;
	c->pre_bit_error.stat[0].scale = FE_SCALE_COUNTER;
	c->pre_bit_count.len = 1;
	c->pre_bit_count.stat[0].scale = FE_SCALE_COUNTER;
	c->block_error.len = 1;
	c->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	c->block_count.len = 1;
	c->block_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;

	ret = AVL6381_Initialize(priv);
	
	return ret;
}

/*static int avl6381_set_sleep(struct dvb_frontend *fe)
{
	int ret;
	struct avl6381_priv *priv = fe->demodulator_priv;
	
	mutex_lock(&priv->mutex);
	ret = AVL6381_Sleep(priv);
	mutex_unlock(&priv->mutex);
	
	return ret;
}*/

static void avl6381_release(struct dvb_frontend *fe)
{
	struct avl6381_priv *priv = fe->demodulator_priv;
	mutex_destroy(&priv->mutex);
	kfree(priv);
	return;
}

static struct dvb_frontend_ops avl6381_ops = {
	.delsys = {SYS_DVBT2, SYS_DVBC_ANNEX_A},
	.info = {
		.name			= "Availink AVL6381 DVBC/DTMB/DVBT2 demodulator",
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
		.frequency_min	= 42000000,
		.frequency_max	= 858000000,
		.frequency_stepsize	= 0,
		.frequency_tolerance	= 0,
#else
		.frequency_min_hz	= 42 * MHz,
		.frequency_max_hz	= 858 * MHz,
		.frequency_stepsize_hz	= 0,
		.frequency_tolerance_hz	= 0,
#endif
		.symbol_rate_min	= 1000000,
		.symbol_rate_max	= 45000000,
		.caps = FE_CAN_FEC_1_2                 |
			FE_CAN_FEC_2_3                 |
			FE_CAN_FEC_3_4                 |
//			FE_CAN_FEC_4_5                 |
			FE_CAN_FEC_5_6                 |
			FE_CAN_FEC_6_7                 |
			FE_CAN_FEC_7_8                 |
			FE_CAN_FEC_AUTO                |
			FE_CAN_QPSK                    |
			FE_CAN_QAM_16                  |
			FE_CAN_QAM_32                  |
			FE_CAN_QAM_64                  |
			FE_CAN_QAM_128                 |
			FE_CAN_QAM_256                 |
			FE_CAN_QAM_AUTO                |
			FE_CAN_TRANSMISSION_MODE_AUTO  |
			FE_CAN_GUARD_INTERVAL_AUTO     |
			FE_CAN_HIERARCHY_AUTO          |
			FE_CAN_MUTE_TS                 |
//			FE_CAN_2G_MODULATION           |
			FE_CAN_MULTISTREAM             |
			FE_CAN_INVERSION_AUTO          |
			FE_CAN_RECOVER                    |
			FE_HAS_EXTENDED_CAPS
	},

	.release					= avl6381_release,
	.init							= avl6381_init,
//	.sleep					= avl6381_set_sleep,
	.i2c_gate_ctrl		= avl6381_i2c_gate_ctrl,
	.read_status			= avl6381_read_status,
//	.get_frontend_algo		= avl6862fe_algo,
	.set_frontend			= avl6381_set_frontend,
};

struct dvb_frontend *avl6381_attach(struct avl6381_config *config,
					struct i2c_adapter *i2c)
{
	struct avl6381_priv *priv;
	int ret;
	u32 id, fid;

	priv = kzalloc(sizeof(struct avl6381_priv), GFP_KERNEL);
	if (priv == NULL)
		goto err;


		memcpy(&priv->frontend.ops, &avl6381_ops, sizeof(struct dvb_frontend_ops));
	
	priv->frontend.demodulator_priv = priv;
	priv->config = config;
	priv->i2c = i2c;
	priv->delivery_system = -1;
	priv->inited = 0;

		if (ret) {
			dev_err(&priv->i2c->dev, "%s: attach failed reading id",
					KBUILD_MODNAME);
			goto err1;
		}

	ret = AVL6381_RD_REG16(priv, 0x040000, &id);

	if (id==0x6381) {

		// get chip id
		ret = AVL6381_RD_REG32(priv, 0x108004, &id);
		// get chip family id
		ret |= AVL6381_RD_REG32(priv, 0x40000, &fid);
			if (fid != 0x63814e24) {
			dev_err(&priv->i2c->dev, "%s: attach failed family id mismatch",
					KBUILD_MODNAME);
			goto err1;
		}
		switch (id) {
			case 0xe: id = 6381; break;
		}
	} else {
		if (fid != 0x68624955) {
			dev_err(&priv->i2c->dev, "%s: attach failed family id mismatch",
					KBUILD_MODNAME);
			goto err1;
		}
		switch (id) {
			case 0xb: id = 6882; break;
			case 0xd: id = 6812; break;
			case 0xe: id = 6762; break;
			case 0xf: id = 6862; break;
		}
	}
	
	dev_info(&priv->i2c->dev, "%s: found AVL%d " \
				"family_id=0x%x", KBUILD_MODNAME, id, fid);

	ret = AVL6381_Initialize(priv);

  return &priv->frontend;
	
err1:
	kfree(priv);
err:
	return NULL;
}

EXPORT_SYMBOL_GPL(avl6381_attach);

MODULE_DESCRIPTION("Availink avl6381 DVB demodulator driver");
MODULE_AUTHOR("Xiaodong Ni <nxiaodong520@gmail.com>");
MODULE_LICENSE("GPL");
