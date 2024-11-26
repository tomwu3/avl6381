// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ITE IT930x driver
 *
 * Copyright (C) 2024 Xiaodong Ni <nxiaodong520@gmail.com>
 */

#include <linux/version.h>
#include "it930x.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)
#define USB_PID_ITETECH_IT9303				0x9306
#endif

static struct avl6381_config avl6381cfg = {
	.demod_address = 0x14,
	.tuner_address = 0x60,
};

static struct mxl603_config mxl608cfg = {
	.singleSupply_3_3V = MXL_ENABLE,
	.xtalCfg.xtalFreqSel = MXL603_XTAL_24MHz,
	.xtalCfg.xtalCap = 12,
	.xtalCfg.clkOutEnable = MXL_ENABLE,
	.xtalCfg.clkOutDiv = MXL_DISABLE,
	.xtalCfg.clkOutExt = MXL_DISABLE,
	.xtalCfg.singleSupply_3_3V = MXL_ENABLE,
	.xtalCfg.XtalSharingMode = MXL_DISABLE,
	.ifOutCfg.ifOutFreq = MXL603_IF_5MHz,
	.ifOutCfg.ifInversion = MXL_ENABLE,
	.ifOutCfg.gainLevel = 11,
	.ifOutCfg.manualFreqSet = MXL_DISABLE, //MXL_ENABLE
	.ifOutCfg.manualIFOutFreqInKHz = 5000,
	.agcCfg.agcType = MXL603_AGC_EXTERNAL,
	.agcCfg.setPoint = 66,
	.agcCfg.agcPolarityInverstion = MXL_DISABLE,
	.tunerModeCfg.signalMode = MXL603_DIG_DVB_C,
	.tunerModeCfg.ifOutFreqinKHz = 5000,
	.tunerModeCfg.xtalFreqSel = MXL603_XTAL_16MHz, //MXL603_XTAL_24MHz,
	.tunerModeCfg.ifOutGainLevel = 11,
};

/*
 * The I2C speed register is calculated with:
 *	I2C speed register = (1000000000 / (24.4 * 16 * I2C_speed))
 *
 * The default speed register for it930x is 7, with means a
 * speed of ~366 kbps
 */
#define I2C_SPEED_366K 7

/* Max transfer size done by I2C transfer functions */
#define MAX_XFER_SIZE  64

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

static u16 it930x_checksum(const u8 *buf, size_t len)
{
	size_t i;
	u16 checksum = 0;

	for (i = 1; i < len; i++) {
		if (i % 2)
			checksum += buf[i] << 8;
		else
			checksum += buf[i];
	}
	checksum = ~checksum;

	return checksum;
}

static int it930x_ctrl_msg(struct dvb_usb_device *d, struct usb_req *req)
{
#define REQ_HDR_LEN 4 /* send header size */
#define ACK_HDR_LEN 3 /* rece header size */
#define CHECKSUM_LEN 2
#define USB_TIMEOUT 2000
	struct state *state = d_to_priv(d);
	int ret, wlen, rlen;
	u16 checksum, tmp_checksum;

	mutex_lock(&d->usb_mutex);

	/* buffer overflow check */
	if (req->wlen > (BUF_LEN - REQ_HDR_LEN - CHECKSUM_LEN) ||
			req->rlen > (BUF_LEN - ACK_HDR_LEN - CHECKSUM_LEN)) {
		dev_err(&d->udev->dev, "too much data wlen=%d rlen=%d\n",
			req->wlen, req->rlen);
		ret = -EINVAL;
		goto exit;
	}

	state->buf[0] = REQ_HDR_LEN + req->wlen + CHECKSUM_LEN - 1;
	state->buf[1] = req->mbox;
	state->buf[2] = req->cmd;
	state->buf[3] = state->seq++;
	memcpy(&state->buf[REQ_HDR_LEN], req->wbuf, req->wlen);

	wlen = REQ_HDR_LEN + req->wlen + CHECKSUM_LEN;
	rlen = ACK_HDR_LEN + req->rlen + CHECKSUM_LEN;

	/* calc and add checksum */
	checksum = it930x_checksum(state->buf, state->buf[0] - 1);
	state->buf[state->buf[0] - 1] = (checksum >> 8);
	state->buf[state->buf[0] - 0] = (checksum & 0xff);

	/* no ack for these packets */
	if (req->cmd == CMD_FW_DL)
		rlen = 0;
	ret = dvb_usbv2_generic_rw_locked(d,
			state->buf, wlen, state->buf, rlen);

			
	if (ret)
		goto exit;

	/* no ack for those packets */
	if (req->cmd == CMD_FW_DL)
		goto exit;

	/* verify checksum */
	checksum = it930x_checksum(state->buf, rlen - 2);
	tmp_checksum = (state->buf[rlen - 2] << 8) | state->buf[rlen - 1];
	if (tmp_checksum != checksum) {
		dev_err(&d->udev->dev, "command=%02x checksum mismatch (%04x != %04x)\n",
			req->cmd, tmp_checksum, checksum);
		ret = -EIO;
		goto exit;
	}

	/* check status */
	if (state->buf[2]) {
		/* fw returns status 1 when IR code was not received */
		if (req->cmd == CMD_IR_GET || state->buf[2] == 1) {
			ret = 1;
			goto exit;
		}

		dev_dbg(&d->udev->dev, "command=%02x failed fw error=%d\n",
			req->cmd, state->buf[2]);
		ret = -EIO;
		goto exit;
	}

	/* read request, copy returned data to return buf */
	if (req->rlen)
		memcpy(req->rbuf, &state->buf[ACK_HDR_LEN], req->rlen);
exit:
	mutex_unlock(&d->usb_mutex);
	return ret;
}

/* write multiple registers */
static int it930x_wr_regs(struct dvb_usb_device *d, u32 reg, u8 *val, int len)
{
	u8 wbuf[MAX_XFER_SIZE];
	u8 mbox = (reg >> 16) & 0xff;
	struct usb_req req = { CMD_MEM_WR, mbox, 6 + len, wbuf, 0, NULL };

	if (6 + len > sizeof(wbuf)) {
		dev_warn(&d->udev->dev, "i2c wr: len=%d is too big!\n", len);
		return -EOPNOTSUPP;
	}

	wbuf[0] = len;
	wbuf[1] = 2;
	wbuf[2] = 0;
	wbuf[3] = 0;
	wbuf[4] = (reg >> 8) & 0xff;
	wbuf[5] = (reg >> 0) & 0xff;
	memcpy(&wbuf[6], val, len);

	return it930x_ctrl_msg(d, &req);
}

/* read multiple registers */
static int it930x_rd_regs(struct dvb_usb_device *d, u32 reg, u8 *val, int len)
{
	u8 wbuf[] = { len, 2, 0, 0, (reg >> 8) & 0xff, reg & 0xff };
	u8 mbox = (reg >> 16) & 0xff;
	struct usb_req req = { CMD_MEM_RD, mbox, sizeof(wbuf), wbuf, len, val };

	return it930x_ctrl_msg(d, &req);
}

/* write single register */
static int it930x_wr_reg(struct dvb_usb_device *d, u32 reg, u8 val)
{
	return it930x_wr_regs(d, reg, &val, 1);
}

/* read single register */
static int it930x_rd_reg(struct dvb_usb_device *d, u32 reg, u8 *val)
{
	return it930x_rd_regs(d, reg, val, 1);
}

/* write single register with mask */
static int it930x_wr_reg_mask(struct dvb_usb_device *d, u32 reg, u8 val,
		u8 mask)
{
	int ret;
	u8 tmp;

	/* no need for read if whole reg is written */
	if (mask != 0xff) {
		ret = it930x_rd_regs(d, reg, &tmp, 1);
		if (ret)
			return ret;

		val &= mask;
		tmp &= ~mask;
		val |= tmp;
	}

	return it930x_wr_regs(d, reg, &val, 1);
}

static int it930x_i2c_read(struct dvb_usb_device *d, u8 addr, u8 *val, int len)
{
	int ret;
	u8 buf[3];
	struct usb_req req = { CMD_GENERIC_I2C_RD, 0, sizeof(buf),
						buf, len, val };
	
	req.mbox |= ((addr & 0x80)  >>  3);
	buf[0] = len;
	buf[1] = 0x01;					
	buf[2] = addr;
	ret = it930x_ctrl_msg(d, &req);
	
	return ret;
}

static int it930x_i2c_write(struct dvb_usb_device *d, u8 addr, u8 *data, int len)
{
	int ret;
	u8 buf[MAX_XFER_SIZE];
	struct usb_req req = { CMD_GENERIC_I2C_WR, 0, 5+len,
						data, 0, NULL };
	
	req.mbox |= ((addr & 0x80)  >>  3);
	buf[0] = len;
	buf[1] = 0x01;					
	buf[2] = addr;
	ret = it930x_ctrl_msg(d, &req);
	
	return ret;
}

static int it930x_i2c_master_xfer(struct i2c_adapter *adap,
		struct i2c_msg msg[], int num)
{
	struct dvb_usb_device *d = i2c_get_adapdata(adap);
	struct state *state = d_to_priv(d);
	int ret;

	if (mutex_lock_interruptible(&d->i2c_mutex) < 0)
		return -EAGAIN;

	/*
	 * AF9035 I2C sub header is 5 bytes long. Meaning of those bytes are:
	 * 0: data len
	 * 1: I2C addr << 1
	 * 2: reg addr len
	 *    byte 3 and 4 can be used as reg addr
	 * 3: reg addr MSB
	 *    used when reg addr len is set to 2
	 * 4: reg addr LSB
	 *    used when reg addr len is set to 1 or 2
	 *
	 * For the simplify we do not use register addr at all.
	 * NOTE: As a firmware knows tuner type there is very small possibility
	 * there could be some tuner I2C hacks done by firmware and this may
	 * lead problems if firmware expects those bytes are used.
	 *
	 * TODO: Here is few hacks. AF9035 chip integrates AF9033 demodulator.
	 * IT9135 chip integrates AF9033 demodulator and RF tuner. For dual
	 * tuner devices, there is also external AF9033 demodulator connected
	 * via external I2C bus. All AF9033 demod I2C traffic, both single and
	 * dual tuner configuration, is covered by firmware - actual USB IO
	 * looks just like a memory access.
	 * In case of IT913x chip, there is own tuner driver. It is implemented
	 * currently as a I2C driver, even tuner IP block is likely build
	 * directly into the demodulator memory space and there is no own I2C
	 * bus. I2C subsystem does not allow register multiple devices to same
	 * bus, having same slave address. Due to that we reuse demod address,
	 * shifted by one bit, on that case.
	 *
	 * For IT930x we use a different command and the sub header is
	 * different as well:
	 * 0: data len
	 * 1: I2C bus (0x03 seems to be only value used)
	 * 2: I2C addr << 1
	 */
#define it930x_IS_I2C_XFER_WRITE_READ(_msg, _num) \
	(_num == 2 && !(_msg[0].flags & I2C_M_RD) && (_msg[1].flags & I2C_M_RD))
#define it930x_IS_I2C_XFER_WRITE(_msg, _num) \
	(_num == 1 && !(_msg[0].flags & I2C_M_RD))
#define it930x_IS_I2C_XFER_READ(_msg, _num) \
	(_num == 1 && (_msg[0].flags & I2C_M_RD))

	if (it930x_IS_I2C_XFER_WRITE_READ(msg, num)) {
		if (msg[0].len > MAX_XFER_SIZE || msg[1].len > MAX_XFER_SIZE) {
			/* TODO: correct limits > 40 */
			ret = -EOPNOTSUPP;
		} else {
			/* I2C write + read */
			u8 buf[MAX_XFER_SIZE];
			struct usb_req req = { CMD_I2C_RD, 0, 5 + msg[0].len,
					buf, msg[1].len, msg[1].buf };

			if (state->chip_type == 0x9306) {
				req.cmd = CMD_GENERIC_I2C_RD;
				req.wlen = 3 + msg[0].len;
			}
			req.mbox |= ((msg[0].addr & 0x80)  >>  3);

			buf[0] = msg[1].len;
			if (state->chip_type == 0x9306) {
				buf[1] = 0x01; /* I2C bus */
				buf[2] = msg[0].addr << 1;
				memcpy(&buf[3], msg[0].buf, msg[0].len);
			} else {
				buf[1] = msg[0].addr << 1;
				buf[3] = 0x00; /* reg addr MSB */
				buf[4] = 0x00; /* reg addr LSB */

				/* Keep prev behavior for write req len > 2*/
				if (msg[0].len > 2) {
					buf[2] = 0x00; /* reg addr len */
					memcpy(&buf[5], msg[0].buf, msg[0].len);

				/* Use reg addr fields if write req len <= 2 */
				} else {
					req.wlen = 5;
					buf[2] = msg[0].len;
					if (msg[0].len == 2) {
						buf[3] = msg[0].buf[0];
						buf[4] = msg[0].buf[1];
					} else if (msg[0].len == 1) {
						buf[4] = msg[0].buf[0];
					}
				}
			}
			ret = it930x_ctrl_msg(d, &req);
		}
	} else if (it930x_IS_I2C_XFER_WRITE(msg, num)) {
		if (msg[0].len > MAX_XFER_SIZE) {
			/* TODO: correct limits > 40 */
			ret = -EOPNOTSUPP;
		} else {
			/* I2C write */
			u8 buf[MAX_XFER_SIZE];
			struct usb_req req = { CMD_I2C_WR, 0, 5 + msg[0].len,
					buf, 0, NULL };

			if (state->chip_type == 0x9306) {
				req.cmd = CMD_GENERIC_I2C_WR;
				req.wlen = 3 + msg[0].len;
			}

			req.mbox |= ((msg[0].addr & 0x80)  >>  3);
			buf[0] = msg[0].len;
			if (state->chip_type == 0x9306) {
				buf[1] = 0x01; /* I2C bus */
				buf[2] = msg[0].addr << 1;
				memcpy(&buf[3], msg[0].buf, msg[0].len);
			} else {
				buf[1] = msg[0].addr << 1;
				buf[2] = 0x00; /* reg addr len */
				buf[3] = 0x00; /* reg addr MSB */
				buf[4] = 0x00; /* reg addr LSB */
				memcpy(&buf[5], msg[0].buf, msg[0].len);
			}
			ret = it930x_ctrl_msg(d, &req);
		}
	} else if (it930x_IS_I2C_XFER_READ(msg, num)) {
		if (msg[0].len > MAX_XFER_SIZE) {
			/* TODO: correct limits > 40 */
			ret = -EOPNOTSUPP;
		} else {
			/* I2C read */
			u8 buf[5];
			struct usb_req req = { CMD_I2C_RD, 0, sizeof(buf),
						buf, msg[0].len, msg[0].buf };

			if (state->chip_type == 0x9306) {
				req.cmd = CMD_GENERIC_I2C_RD;
				req.wlen = 3;
			}
			req.mbox |= ((msg[0].addr & 0x80)  >>  3);
			buf[0] = msg[0].len;
			if (state->chip_type == 0x9306) {
				buf[1] = 0x01; /* I2C bus */
				buf[2] = (msg[0].addr << 1);
			} else {
				buf[1] = msg[0].addr << 1;
				buf[2] = 0x00; /* reg addr len */
				buf[3] = 0x00; /* reg addr MSB */
				buf[4] = 0x00; /* reg addr LSB */
			}
			ret = it930x_ctrl_msg(d, &req);
		}
	} else {
		/*
		 * We support only three kind of I2C transactions:
		 * 1) 1 x write + 1 x read (repeated start)
		 * 2) 1 x write
		 * 3) 1 x read
		 */
		ret = -EOPNOTSUPP;
	}

	mutex_unlock(&d->i2c_mutex);

	if (ret < 0)
		return ret;
	else
		return num;
}

static u32 it930x_i2c_functionality(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C;
}

static struct i2c_algorithm it930x_i2c_algo = {
	.master_xfer = it930x_i2c_master_xfer,
	.functionality = it930x_i2c_functionality,
};

int it930x_set_gpio(struct dvb_usb_device *d, u32 gpio_reg, u8 val) {
	int ret = 0;

	ret = it930x_wr_reg(d, gpio_reg, val);

	return ret;
}

int it930x_set_gpio_mode(struct dvb_usb_device *d, enum it930x_gpio gpio, enum it930x_gpio_mode mode)
{
	/* GPIO mode registers (gpioh1 to gpioh16) */
	u32 gpio_en_regs[] = {
		0xd8b0,
		0xd8b8,
		0xd8b4,
		0xd8c0,
		0xd8bc,
		0xd8c8,
		0xd8c4,
		0xd8d0,
		0xd8cc,
		0xd8d8,
		0xd8d4,
		0xd8e0,
		0xd8dc,
		0xd8e4,
		0xd8e8,
		0xd8ec,
	};
	return it930x_set_gpio(d, gpio_en_regs[gpio], mode);
}

int it930x_enable_gpio(struct dvb_usb_device *d, enum it930x_gpio gpio, bool enable)
{
	/* GPIO enable/disable registers (gpioh1 to gpioh16) */
	u32 gpio_on_regs[] = {
		0xd8b1,
		0xd8b9,
		0xd8b5,
		0xd8c1,
		0xd8bd,
		0xd8c9,
		0xd8c5,
		0xd8d1,
		0xd8cd,
		0xd8d9,
		0xd8d5,
		0xd8e1,
		0xd8dd,
		0xd8e5,
		0xd8e9,
		0xd8ed,
	};
	return it930x_set_gpio(d, gpio_on_regs[gpio], (enable) ? 1 : 0);
}

int it930x_read_gpio(struct dvb_usb_device *d, enum it930x_gpio gpio, bool *high)
{
	/* GPIO input registers (gpioh1 to gpioh16) */
	u32 gpio_i_regs[] = {
		0xd8ae,
		0xd8b6,
		0xd8b2,
		0xd8be,
		0xd8ba,
		0xd8c6,
		0xd8c2,
		0xd8ce,
		0xd8ca,
		0xd8d6,
		0xd8d2,
		0xd8de,
		0xd8da,
		0xd8e2,
		0xd8e6,
		0xd8ea,
	};
	int ret = 0;
	u8 tmp;

	mutex_lock(&d->usb_mutex);

	ret = it930x_rd_reg(d, gpio_i_regs[gpio], &tmp);
	if (!ret)
		*high = (tmp) ? true : false;

	mutex_unlock(&d->usb_mutex);

	return ret;
}

int it930x_write_gpio(struct dvb_usb_device *d, enum it930x_gpio gpio, bool high)
{
	/* GPIO output registers (gpioh1 to gpioh16) */
	u32 gpio_o_regs[] = {
		0xd8af,
		0xd8b7,
		0xd8b3,
		0xd8bf,
		0xd8bb,
		0xd8c7,
		0xd8c3,
		0xd8cf,
		0xd8cb,
		0xd8d7,
		0xd8d3,
		0xd8df,
		0xd8db,
		0xd8e3,
		0xd8e7,
		0xd8eb,
	};
	return it930x_set_gpio(d, gpio_o_regs[gpio], (high) ? 1 : 0);
}

static int it930x_download_firmware_old(struct dvb_usb_device *d,
		const struct firmware *fw)
{
	int ret, i, j, len;
	u8 wbuf[1];
	struct usb_req req = { 0, 0, 0, NULL, 0, NULL };
	struct usb_req req_fw_dl = { CMD_FW_DL, 0, 0, wbuf, 0, NULL };
	u8 hdr_core;
	u16 hdr_addr, hdr_data_len, hdr_checksum;
	#define MAX_DATA 58
	#define HDR_SIZE 7

	/*
	 * Thanks to Daniel Glöckner <daniel-gl@gmx.net> about that info!
	 *
	 * byte 0: MCS 51 core
	 *  There are two inside the AF9035 (1=Link and 2=OFDM) with separate
	 *  address spaces
	 * byte 1-2: Big endian destination address
	 * byte 3-4: Big endian number of data bytes following the header
	 * byte 5-6: Big endian header checksum, apparently ignored by the chip
	 *  Calculated as ~(h[0]*256+h[1]+h[2]*256+h[3]+h[4]*256)
	 */

	for (i = fw->size; i > HDR_SIZE;) {
		hdr_core = fw->data[fw->size - i + 0];
		hdr_addr = fw->data[fw->size - i + 1] << 8;
		hdr_addr |= fw->data[fw->size - i + 2] << 0;
		hdr_data_len = fw->data[fw->size - i + 3] << 8;
		hdr_data_len |= fw->data[fw->size - i + 4] << 0;
		hdr_checksum = fw->data[fw->size - i + 5] << 8;
		hdr_checksum |= fw->data[fw->size - i + 6] << 0;

		dev_dbg(&d->udev->dev, "core=%d addr=%04x data_len=%d checksum=%04x\n",
			hdr_core, hdr_addr, hdr_data_len, hdr_checksum);

		if (((hdr_core != 1) && (hdr_core != 2)) ||
				(hdr_data_len > i)) {
			dev_dbg(&d->udev->dev, "bad firmware\n");
			break;
		}

		/* download begin packet */
		req.cmd = CMD_FW_DL_BEGIN;
		ret = it930x_ctrl_msg(d, &req);
		if (ret < 0)
			goto err;

		/* download firmware packet(s) */
		for (j = HDR_SIZE + hdr_data_len; j > 0; j -= MAX_DATA) {
			len = j;
			if (len > MAX_DATA)
				len = MAX_DATA;
			req_fw_dl.wlen = len;
			req_fw_dl.wbuf = (u8 *) &fw->data[fw->size - i +
					HDR_SIZE + hdr_data_len - j];
			ret = it930x_ctrl_msg(d, &req_fw_dl);
			if (ret < 0)
				goto err;
		}

		/* download end packet */
		req.cmd = CMD_FW_DL_END;
		ret = it930x_ctrl_msg(d, &req);
		if (ret < 0)
			goto err;

		i -= hdr_data_len + HDR_SIZE;

		dev_dbg(&d->udev->dev, "data uploaded=%zu\n", fw->size - i);
	}

	/* print warn if firmware is bad, continue and see what happens */
	if (i)
		dev_warn(&d->udev->dev, "bad firmware\n");

	return 0;

err:
	dev_dbg(&d->udev->dev, "failed=%d\n", ret);

	return ret;
}

static int it930x_download_firmware_new(struct dvb_usb_device *d,
		const struct firmware *fw)
{
	int ret, i, i_prev;
	struct usb_req req_fw_dl = { CMD_FW_SCATTER_WR, 0, 0, NULL, 0, NULL };
	#define HDR_SIZE 7

	/*
	 * There seems to be following firmware header. Meaning of bytes 0-3
	 * is unknown.
	 *
	 * 0: 3
	 * 1: 0, 1
	 * 2: 0
	 * 3: 1, 2, 3
	 * 4: addr MSB
	 * 5: addr LSB
	 * 6: count of data bytes ?
	 */
	for (i = HDR_SIZE, i_prev = 0; i <= fw->size; i++) {
		if (i == fw->size ||
				(fw->data[i + 0] == 0x03 &&
				(fw->data[i + 1] == 0x00 ||
				fw->data[i + 1] == 0x01) &&
				fw->data[i + 2] == 0x00)) {
			req_fw_dl.wlen = i - i_prev;
			req_fw_dl.wbuf = (u8 *) &fw->data[i_prev];
			i_prev = i;
			ret = it930x_ctrl_msg(d, &req_fw_dl);
			if (ret < 0)
				goto err;

			dev_dbg(&d->udev->dev, "data uploaded=%d\n", i);
		}
	}

	return 0;

err:
	dev_dbg(&d->udev->dev, "failed=%d\n", ret);

	return ret;
}

static int it930x_download_firmware(struct dvb_usb_device *d,
		const struct firmware *fw)
{
	struct state *state = d_to_priv(d);
	int ret;
	u8 wbuf[1];
	u8 rbuf[4];
//	u8 tmp;
	struct usb_req req = { 0, 0, 0, NULL, 0, NULL };
	struct usb_req req_fw_ver = { CMD_FW_QUERYINFO, 0, 1, wbuf, 4, rbuf };

	dev_dbg(&d->udev->dev, "\n");

	if (fw->data[0] == 0x01)
		ret = it930x_download_firmware_old(d, fw);
	else
		ret = it930x_download_firmware_new(d, fw);
	if (ret < 0)
		goto err;

	/* firmware loaded, request boot */
	req.cmd = CMD_FW_BOOT;
	ret = it930x_ctrl_msg(d, &req);
	if (ret < 0)
		goto err;

	/* ensure firmware starts */
	wbuf[0] = 1;
	ret = it930x_ctrl_msg(d, &req_fw_ver);
	if (ret < 0)
		goto err;

	if (!(rbuf[0] || rbuf[1] || rbuf[2] || rbuf[3])) {
		dev_err(&d->udev->dev, "firmware did not run\n");
		ret = -ENODEV;
		goto err;
	}

	dev_info(&d->udev->dev, "firmware version=%d.%d.%d.%d",
		 rbuf[0], rbuf[1], rbuf[2], rbuf[3]);

	return 0;

err:
	dev_dbg(&d->udev->dev, "failed=%d\n", ret);

	return ret;
}

static int it930x_tuner_select_input(struct dvb_frontend *fe, enum fe_delivery_system delivery_system)
{
  int ret;
  struct dvb_usb_device *d = fe_to_d(fe);

  ret = it930x_set_gpio_mode(d, IT930X_GPIO2, IT930X_GPIO_OUT);
  ret |= it930x_enable_gpio(d, IT930X_GPIO2, IT930X_GPIO_ENABLE);
  ret |= it930x_set_gpio_mode(d, IT930X_GPIO3, IT930X_GPIO_OUT);
  ret |= it930x_enable_gpio(d, IT930X_GPIO3, IT930X_GPIO_ENABLE);
	switch (delivery_system) {  
	case SYS_DVBT:
	case SYS_DVBT2:
		ret |= it930x_write_gpio(d, IT930X_GPIO2, IT930X_GPIO_HIGH);
		ret |= it930x_write_gpio(d, IT930X_GPIO3, IT930X_GPIO_LOW);
    break;
	case SYS_DVBC_ANNEX_A:
		ret |= it930x_write_gpio(d, IT930X_GPIO2, IT930X_GPIO_LOW);
		ret |= it930x_write_gpio(d, IT930X_GPIO3, IT930X_GPIO_LOW);
    break;
  }

  return ret;
}

static int it930x_identify_state(struct dvb_usb_device *d, const char **name)
{
	struct state *state = d_to_priv(d);
	int ret;
	u8 wbuf[1] = { 1 };
	u8 rbuf[4];
	struct usb_req req = { CMD_FW_QUERYINFO, 0, sizeof(wbuf), wbuf,
			sizeof(rbuf), rbuf };

	/* configure gpio1, reset & power slave demod */
	ret = it930x_set_gpio_mode(d, IT930X_GPIO1, IT930X_GPIO_OUT);
	ret |= it930x_enable_gpio(d, IT930X_GPIO1, IT930X_GPIO_ENABLE);
	ret |= it930x_write_gpio(d, IT930X_GPIO1, IT930X_GPIO_LOW);

	msleep(20);
	
	ret = it930x_rd_regs(d, 0x1222, rbuf, 3);
	if (ret < 0)
		goto err;

	state->chip_version = rbuf[0];
	state->chip_type = rbuf[2] << 8 | rbuf[1] << 0;

	ret = it930x_rd_reg(d, 0x384f, &state->prechip_version);
	if (ret < 0)
		goto err;

	dev_info(&d->udev->dev, "prechip_version=%02x chip_version=%02x chip_type=%04x\n",
		 state->prechip_version, state->chip_version, state->chip_type);

	if (state->chip_type == 0x9306) {
		*name = ITE_FIRMWARE_IT9303;
	}
	
	ret |= it930x_ctrl_msg(d, &req);

	msleep(7);
	ret |=  it930x_wr_reg_mask(d, 0xda05, 0x01, 0x01);		//check
	ret |= it930x_write_gpio(d, IT930X_GPIO1, IT930X_GPIO_HIGH);
	
	//mp2_sw_rst, reset EP4
	ret |= it930x_wr_reg(d, 0xda1d,  0x01);
	msleep(2);
	ret |= it930x_wr_reg(d, 0xda1d,  0x00);

	msleep(8);
	ret |= it930x_wr_reg(d, 0x4976,  0x00);
	ret |= it930x_wr_reg(d, 0x4bfb,  0x00);
	ret |= it930x_wr_reg(d, 0x4978,  0x00);
	ret |= it930x_wr_reg(d, 0x4977,  0x00);

	/* I2C master bus 1,3 clock speed 366k */
	ret |= it930x_wr_reg(d, 0xf103, I2C_SPEED_366K);

	if (ret)
		goto err;

	dev_info(&d->udev->dev, "reply=%*ph\n", 4, rbuf);
	if (rbuf[0] || rbuf[1] || rbuf[2] || rbuf[3])
		ret = WARM;
	else
		ret = COLD;

	return ret;

err:
	dev_dbg(&d->udev->dev, "failed=%d\n", ret);

	return ret;
}

static int it930x_init(struct dvb_usb_device *d)
{
	int ret, i;
	u8 tmp;
	u16 frame_size = (d->udev->speed == USB_SPEED_FULL ? 5 : 816) * 188 / 4;
	u8 packet_size = (d->udev->speed == USB_SPEED_FULL ? 64 : 512) / 4;
	u8 buf[3];

	/* I2C master bus 2 clock speed 366k */
	ret = it930x_wr_reg(d, 0xf6a7, I2C_SPEED_366K);

	/* I2C master bus 1,3 clock speed 366k */
	ret |= it930x_wr_reg(d, 0xf103, I2C_SPEED_366K);
		
	/* ignore sync byte: no */
	ret |= it930x_wr_reg(d, 0xda1a, 0);

	/* dvb-t interrupt: enable */
	ret |= it930x_wr_reg_mask(d, 0xf41f, 0x04, 0x04);

	/* mpeg full speed */
	ret |= it930x_wr_reg_mask(d, 0xda10, 0x00, 0x00);

	/* dvb-t mode: enable */
	ret |= it930x_wr_reg_mask(d, 0xf41a, 0x05, 0x05);		//?
		
	ret |= it930x_wr_reg_mask(d, 0xda1d, 0x01, 0x01);
	
	/* enable ep4 */
	ret |= it930x_wr_reg_mask(d, 0xdd11, 0x0F, 0x0F);		//?

	/* disable nak of ep4 */
	ret |= it930x_wr_reg_mask(d, 0xdd13, 0x1b, 0x1b);		//?

	/* enable ep4 */
	ret |= it930x_wr_reg_mask(d, 0xdd11, 0x2F, 0x2F);		//?

	buf[0] = (frame_size & 0xff);
	buf[1] = ((frame_size >> 8) & 0xff);
	ret |= it930x_wr_regs(d, 0xdd88, buf, 2);
//	ret |= it930x_wr_reg(d, 0xdd88, (frame_size >> 0) & 0xff);		//check ??

	/* max bulk packet size */
	ret |= it930x_wr_reg(d, 0xdd0c, packet_size);
	
	ret |= it930x_wr_reg_mask(d, 0xda05, 0x00, 0x01);
	ret |= it930x_wr_reg_mask(d, 0xda06, 0x00, 0x01);
	ret |= it930x_wr_reg_mask(d, 0xda1d, 0x00, 0x01);
	
	/* reverse: no */
	ret |= it930x_wr_reg(d, 0xd920, 0x00);
	
	/* power config ? */
	ret |= it930x_wr_reg(d, 0xd833, 0x01);

	ret |= it930x_wr_reg(d, 0xd830, 0x00);
	ret |= it930x_wr_reg(d, 0xd831, 0x01);
	ret |= it930x_wr_reg(d, 0xd832, 0x00);
	ret |= it930x_wr_reg(d, 0x4976, 0x01);

/*	msleep(7);
	//init it9300  eeprom  //IT9300_cm_init
	ret |= it930x_wr_reg(d, 0x4960,  0x0A);			//check ??
	ret |= it930x_wr_reg(d, 0x495f,  0xB0);
	ret |= it930x_write_gpio(d, IT930X_GPIO15, IT930X_GPIO_HIGH);
  ret |= it930x_set_gpio_mode(d, IT930X_GPIO15, IT930X_GPIO_OUT);
  ret |= it930x_enable_gpio(d, IT930X_GPIO15, IT930X_GPIO_ENABLE);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_HIGH);
  ret |= it930x_set_gpio_mode(d, IT930X_GPIO14, IT930X_GPIO_OUT);
  ret |= it930x_enable_gpio(d, IT930X_GPIO14, IT930X_GPIO_ENABLE);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_LOW);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_HIGH);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_LOW);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_HIGH);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_LOW);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_HIGH);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_LOW);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_HIGH);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_LOW);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_HIGH);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_LOW);
	ret |= it930x_write_gpio(d, IT930X_GPIO14, IT930X_GPIO_HIGH);*/

	msleep(20);
	ret |= it930x_wr_reg_mask(d, 0xda58, 0x00, 0x01);	//check ??			//ts_in_src, serial
	msleep(8);
	ret |= it930x_wr_reg(d, 0xda51, 0x00);									//in ts pkt len
	msleep(8);
	ret |= it930x_wr_reg(d, 0xda73, 0x01);									//ts0_aggre_mode
	ret |= it930x_wr_reg(d, 0xda78, 0x47);									//ts0_sync_byte
	msleep(30);
	ret |= it930x_wr_reg(d, 0xda4c, 0x01);									//ts0_en
	msleep(8);
	ret |= it930x_wr_reg(d, 0xda5a, 0x1F);									//ts_fail_ignore

/*	ret |= it930x_rd_reg(d, 0xd800, &tmp);							//rd teststrap regiater //get demod clock
	ret |= it930x_rd_reg(d, 0xd801, &tmp);							//rd poweron_bootstrap regiater
	ret |= it930x_rd_reg(d, 0xd802, &tmp);							//rd poweron_hostboot regiater
	ret |= it930x_rd_reg(d, 0xda20, &tmp);							//rd poweron_usbmode regiater
	msleep(20);
//return 0;
	buf[0] = 0xc3;
	buf[1] = 0x87;
	buf[2] = 0x00;
	ret |= it930x_wr_regs(d, 0x4900, buf, 3);						//IT9300_cm_sendData  IT9300_cm_sendCommand  IT9300_cm_sendCmdByte  IT9300_cm_receiveData
//	ret |= it930x_wr_regs(d, 0x4900, 0xc38700, 3);
	msleep(1);
	ret |= it930x_rd_reg(d, 0x4961, &tmp);							//IT9300_cm_sendData  IT9300_cm_sendCommand  IT9300_cm_sendCmdByte

	u8 data[MAX_XFER_SIZE];	
	ret |= it930x_i2c_read(d, 0x61, data, 2);
	dev_info(&d->udev->dev, "read_61:0x%02x%02x", data[0], data[1]);
	ret |= it930x_i2c_read(d, 0x6f, data, 1);
	dev_info(&d->udev->dev, "read_6f:0x%02x", data[0]);
	ret |= it930x_i2c_read(d, 0x61, data, 2);
	dev_info(&d->udev->dev, "read_61:0x%02x%02x", data[0], data[1]);
	ret |= it930x_i2c_read(d, 0x61, data, 2);
	dev_info(&d->udev->dev, "read_61:0x%02x%02x", data[0], data[1]);
	ret |= it930x_i2c_read(d, 0x61, data, 2);
	dev_info(&d->udev->dev, "read_61:0x%02x%02x", data[0], data[1]);
	ret |= it930x_i2c_read(d, 0x65, data, 32);
	dev_info(&d->udev->dev, "read_65:0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x...", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);
*/	
	if (ret < 0)
		goto err;

	return 0;

err:
	dev_dbg(&d->udev->dev, "failed=%d\n", ret);

	return ret;
}

static int it930x_frontend_attach(struct dvb_usb_adapter *adap)
{
//	struct state *state = adap_to_priv(adap);
	struct dvb_usb_device *d = adap_to_d(adap);
	int ret = 0;
	
  ret = it930x_set_gpio_mode(d, IT930X_GPIO1, IT930X_GPIO_OUT);
  ret |= it930x_enable_gpio(d, IT930X_GPIO1, IT930X_GPIO_ENABLE);
	ret |= it930x_write_gpio(d, IT930X_GPIO1, IT930X_GPIO_LOW);
	msleep(30);
	ret |= it930x_write_gpio(d, IT930X_GPIO1, IT930X_GPIO_HIGH);
	msleep(150);
				 
	dev_info(&d->udev->dev, "Checking for Availink AVL6381 DVB-S2/T2/C demod ...\n");
	avl6381cfg.tuner_select_input = it930x_tuner_select_input;
	adap->fe[0] = dvb_attach(avl6381_attach, &avl6381cfg, &d->i2c_adap);
	if (adap->fe[0] == NULL)
	{
		dev_info(&d->udev->dev, "Failed to find AVL6381 demod!\n");
		ret = -ENODEV;
		goto err;
	}

	return ret;

err:
	dev_dbg(&d->udev->dev, "failed=%d\n", ret);

	return ret;
}

static int it930x_tuner_attach(struct dvb_usb_adapter *adap)
{
//	struct state *state = adap_to_priv(adap);
	struct dvb_usb_device *d = adap_to_d(adap);
	struct dvb_frontend *fe = NULL;
	int ret;
	struct i2c_client *client;

	dev_dbg(&d->udev->dev, "adap->id=%d\n", adap->id);

	fe = dvb_attach(mxl603_attach, adap->fe[0], &d->i2c_adap, avl6381cfg.tuner_address, &mxl608cfg);
	if (fe == NULL)
	{
		ret = -ENODEV;
		goto err;
	}

	return 0;

err:
	dev_info(&d->udev->dev, "failed=%d\n", ret);

	return ret;
}

static int it930x_get_stream_config(struct dvb_frontend *fe, u8 *ts_type,
		struct usb_data_stream_properties *stream)
{
	struct dvb_usb_device *d = fe_to_d(fe);

	dev_dbg(&d->udev->dev, "adap=%d\n", fe_to_adap(fe)->id);

	if (d->udev->speed == USB_SPEED_FULL)
		stream->u.bulk.buffersize = 5 * 188;

	return 0;
}

static int it930x_pid_filter_ctrl(struct dvb_usb_adapter *adap, int onoff)
{
	struct dvb_usb_device *d = adap_to_d(adap);
	u32 remap_mode_regs[5] = {
		0xda13,
		0xda25,
		0xda29,
		0xda2d,
		0xda7f
	};
	int ret = 0;
	u8 port, data[2];

	port = 0;

	/* block or pass */
	ret = it930x_wr_reg(d, remap_mode_regs[port], onoff ? 2 : 0);
	if (ret)
		return ret;

	/* sync_byte and remap */
	ret = it930x_wr_reg(d, 0xda73 + port, 3);
	if (ret)
		return ret;

	data[0] = 0;
	data[1] = 0;

	/* pid offset */
	ret = it930x_wr_regs(d, 0xda81 + (port * 2), data, 2);
	if (ret)
		return ret;

//	dev_info(&d->udev->dev, "it930x_pid_filter_ctrl onoff=%d\n", onoff);
	return 0;
}

static int it930x_pid_filter(struct dvb_usb_adapter *adap, int index, u16 pid,
		int onoff)
{
	struct dvb_usb_device *d = adap_to_d(adap);
	u32 pid_index_regs[5] = {
		0xda15,
		0xda26,
		0xda2a,
		0xda2e,
		0xda80
	};
	int ret = 0;
	u8 port, data[2];

	port = 0;
	data[0] = pid & 0xff;
	data[1] = (pid >> 8) & 0xff;

	/* target pid */
	ret = it930x_wr_regs(d, 0xda16, data, 2);
	if (ret)
		return ret;

	/* enable */
	ret = it930x_wr_reg(d, 0xda14, 1);
	if (ret)
		return ret;

	/* index */
	ret = it930x_wr_reg(d, pid_index_regs[port], index);
	if (ret)
		return ret;
	
//	dev_info(&d->udev->dev, "it930x_pid_filter index=%d pid=%u onoff=%d\n", index, pid, onoff);
	return 0;
}

static const struct dvb_usb_device_properties it930x_props = {
	.driver_name = KBUILD_MODNAME,
	.owner = THIS_MODULE,
	.adapter_nr = adapter_nr,
	.size_of_priv = sizeof(struct state),

	.generic_bulk_ctrl_endpoint = 0x02,
	.generic_bulk_ctrl_endpoint_response = 0x81,

	.identify_state = it930x_identify_state,
	.download_firmware = it930x_download_firmware,

	.i2c_algo = &it930x_i2c_algo,
//	.read_config = it930x_read_config,
	.frontend_attach = it930x_frontend_attach,
	.tuner_attach = it930x_tuner_attach,
	.init = it930x_init,
	.get_stream_config = it930x_get_stream_config,

	.num_adapters = 1,
	.adapter = {
		{
//			.caps = DVB_USB_ADAP_HAS_PID_FILTER,
			.caps = DVB_USB_ADAP_HAS_PID_FILTER |
				DVB_USB_ADAP_PID_FILTER_CAN_BE_TURNED_OFF |
				DVB_USB_ADAP_NEED_PID_FILTERING ,
			.pid_filter_count = 64,
			.pid_filter_ctrl = it930x_pid_filter_ctrl,
			.pid_filter = it930x_pid_filter,
			
			.stream = DVB_USB_STREAM_BULK(0x84, 4, 816 * 188),
//		}, {
//			.stream = DVB_USB_STREAM_BULK(0x85, 4, 816 * 188),
		},
	},
};

static const struct usb_device_id it930x_id_table[] = {
	/* IT930x devices */
	{ DVB_USB_DEVICE(USB_VID_ITETECH, USB_PID_ITETECH_IT9303,
		&it930x_props, "ITE IT930x Generic", NULL) },
	{ }
};
MODULE_DEVICE_TABLE(usb, it930x_id_table);

static struct usb_driver it930x_usb_driver = {
	.name = KBUILD_MODNAME,
	.id_table = it930x_id_table,
	.probe = dvb_usbv2_probe,
	.disconnect = dvb_usbv2_disconnect,
	.suspend = dvb_usbv2_suspend,
	.resume = dvb_usbv2_resume,
	.reset_resume = dvb_usbv2_reset_resume,
	.no_dynamic_id = 1,
	.soft_unbind = 1,
};

module_usb_driver(it930x_usb_driver);

MODULE_DESCRIPTION("ITE IT930x driver");
MODULE_AUTHOR("Xiaodong Ni <nxiaodong520@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_FIRMWARE(ITE_FIRMWARE_IT9303);