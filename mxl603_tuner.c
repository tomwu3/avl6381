// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * MaxLinear MXL603 tuner driver
 *
 * Copyright (C) 2024 Xiaodong Ni <nxiaodong520@gmail.com>
 */

#include <linux/i2c.h>
#include <linux/types.h>
#include "tuner-i2c.h"
//#include "mxl603_api.h"
#include "mxl603_tuner.h"

struct mxl603_state {
	struct mxl603_config *config;
	struct i2c_adapter   *i2c;
	u8 addr;
	u32 frequency;
	u32 bandwidth;
};

static int mxl603_synth_lock_status(struct mxl603_state *state, int *rf_locked, int *ref_locked)
{
	u8 d = 0;
	int ret;
	MXL_BOOL rfLockPtr, refLockPtr;

	*rf_locked = 0;
	*ref_locked = 0;

	ret = MxLWare603_API_ReqTunerLockStatus(state->i2c, state->addr, &rfLockPtr, &refLockPtr);
	if (ret)
		goto err;

	if (rfLockPtr)
		*rf_locked = 1;

	if (refLockPtr)
		*ref_locked = 1;
	
	return 0;
	
err:
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int mxl603_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct mxl603_state *state = fe->tuner_priv;
	int rf_locked, ref_locked, ret;

	*status = 0;

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	ret = mxl603_synth_lock_status(state, &rf_locked, &ref_locked);
	if (ret)
		goto err;

	dev_info(&state->i2c->dev, "%s%s", rf_locked ? "rf locked " : "",
			ref_locked ? "ref locked" : "");

	if ((rf_locked) || (ref_locked))
		*status |= TUNER_STATUS_LOCKED;

		
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return 0;
	
err:
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
		
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int mxl603_get_rf_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct mxl603_state *state = fe->tuner_priv;
	SINT16 rxPwrPtr;
	int ret;
	
	*strength = 0;

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	ret = MxLWare603_API_ReqTunerRxPower(state->i2c, state->addr, &rxPwrPtr);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
	
	if (!ret)
	{
		*strength = ~rxPwrPtr + 1;	
	}
	else
		dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
		
	return ret;
}

static int mxl603_set_params(struct dvb_frontend *fe)
{	
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct mxl603_state *state = fe->tuner_priv;	
//	MXL603_XTAL_SET_CFG_T xtalCfg;
//	MXL603_IF_OUT_CFG_T ifOutCfg;
//	MXL603_AGC_CFG_T agcCfg;
//	MXL603_TUNER_MODE_CFG_T tunerModeCfg;
	MXL603_BW_E bandWidth;
	int ret;
	int rf_locked, ref_locked;
	u32 freq = c->frequency;
	
	dev_info(&state->i2c->dev, 
		"%s: delivery_system=%d frequency=%d bandwidth_hz=%d\n", 
		__func__, c->delivery_system, c->frequency, c->bandwidth_hz);		
			

	switch (c->delivery_system) {
	case SYS_ATSC:
		state->config->tunerModeCfg.signalMode = MXL603_DIG_ISDBT_ATSC;
		bandWidth = MXL603_TERR_BW_6MHz ;
		break;
	case SYS_DVBC_ANNEX_A:
		state->config->tunerModeCfg.signalMode = MXL603_DIG_DVB_C;
		bandWidth = MXL603_CABLE_BW_8MHz;	
		break;
	case SYS_DVBT:
	case SYS_DVBT2:
		state->config->tunerModeCfg.signalMode = MXL603_DIG_DVB_T_DTMB;
		switch (c->bandwidth_hz) {
		case 6000000:
			bandWidth = MXL603_TERR_BW_6MHz;
			break;
		case 7000000:
			bandWidth = MXL603_TERR_BW_7MHz ;
			break;
		case 8000000:
			bandWidth = MXL603_TERR_BW_8MHz ;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		 dev_dbg(&state->i2c->dev, "%s: err state=%d\n", 
			__func__, fe->dtv_property_cache.delivery_system);
		return -EINVAL;
	}

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);
	
/*	ifOutCfg.ifOutFreq = MXL603_IF_5MHz; //we suggest 5Mhz for ATSC MN88436 
	ifOutCfg.ifInversion = MXL_ENABLE;
	ifOutCfg.gainLevel = 11;
	ifOutCfg.manualFreqSet = MXL_DISABLE;
	ifOutCfg.manualIFOutFreqInKHz = 5000;//4984;*/
	ret = MxLWare603_API_CfgTunerIFOutParam(state->i2c, state->addr, state->config->ifOutCfg);
	if (ret)
		goto err;

	//IF freq set, should match Demod request 
//	tunerModeCfg.ifOutFreqinKHz = 5000;

	/* Single XTAL for tuner and demod sharing*/
//	tunerModeCfg.xtalFreqSel = MXL603_XTAL_24MHz;
//	tunerModeCfg.ifOutGainLevel = 11;

	ret = MxLWare603_API_CfgTunerMode(state->i2c, state->addr, state->config->tunerModeCfg);
	if (ret)
		goto err;

	Mxl603SetFreqBw(state->i2c, state->addr, freq, bandWidth, state->config->tunerModeCfg.signalMode);
	if (ret)
		goto err;

	state->frequency = freq;
	state->bandwidth = c->bandwidth_hz;

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
		
	return 0;
	
err:
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
		
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int mxl603_get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct mxl603_state *state = fe->tuner_priv;
	*frequency = state->frequency;
	return 0;
}

static int mxl603_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	struct mxl603_state *state = fe->tuner_priv;
	*bandwidth = state->bandwidth;
	return 0;
}

static int mxl603_tuner_init(struct dvb_frontend *fe)
{
	struct mxl603_state *state = fe->tuner_priv;
	int ret;
	MXL603_VER_INFO_T	mxl603Version;

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	ret = MXL603_init(state->i2c, state->addr, *state->config);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return 0;
	
err:
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
		
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

static int mxl603_sleep(struct dvb_frontend *fe)
{
	struct mxl603_state *state = fe->tuner_priv;
	int ret;

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	/* enter standby mode */
	ret = MxLWare603_API_CfgDevPowerMode(state->i2c, state->addr, MXL603_PWR_MODE_STANDBY);
	
	if (ret)
		goto err;
		
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return 0;
	
err:
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
		
	dev_dbg(&state->i2c->dev, "%s: failed=%d\n", __func__, ret);
	return ret;
}

void mxl603_release(struct dvb_frontend *fe)
{
	struct mxl603_state *state = fe->tuner_priv;

	fe->tuner_priv = NULL;
	kfree(state);
	
	return;
}

static struct dvb_tuner_ops mxl603_tuner_ops = {
	.info = {
		.name = "MaxLinear MXL603",
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
		.frequency_min = 1000000,
		.frequency_max = 1200000000,
		.frequency_step = 25000,
#else
		.frequency_min_hz = 1 * MHz,
		.frequency_max_hz = 1200 * MHz,
		.frequency_step_hz = 25 * kHz,
#endif
	},
	.init              = mxl603_tuner_init,
	.sleep             = mxl603_sleep,
	.set_params        = mxl603_set_params,
//	.get_status        = mxl603_get_status,
	.get_rf_strength   = mxl603_get_rf_strength,
//	.get_frequency     = mxl603_get_frequency,
//	.get_bandwidth     = mxl603_get_bandwidth,
	.release           = mxl603_release,
//	.get_if_frequency  = mxl603_get_if_frequency,
};

struct dvb_frontend *mxl603_attach(struct dvb_frontend *fe,
				     struct i2c_adapter *i2c, u8 addr,
				     struct mxl603_config *config)
{
	struct mxl603_state *state = NULL;
	int ret = 0;
	MXL603_VER_INFO_T	mxl603Version;

	state = kzalloc(sizeof(struct mxl603_state), GFP_KERNEL);
	if (!state) {
		ret = -ENOMEM;
		dev_err(&i2c->dev, "kzalloc() failed\n");
		goto err1;
	}
	
	state->config = config;
	state->i2c = i2c;
	state->addr = addr;
	
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);
		
	ret = MxLWare603_API_CfgDevSoftReset(state->i2c, state->addr);

	ret |= MxLWare603_API_ReqDevVersionInfo(state->i2c, state->addr, &mxl603Version);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	dev_info(&state->i2c->dev, "MXL603 detected id(%02x) ver(%02x)\n", mxl603Version.chipId, mxl603Version.chipVersion);

	/* check return value of mxl603_get_chip_id */
	if (ret)
		goto err2;
	
	dev_info(&i2c->dev, "Attaching MXL603\n");
	
	fe->tuner_priv = state;

	memcpy(&fe->ops.tuner_ops, &mxl603_tuner_ops,
	       sizeof(struct dvb_tuner_ops));

	return fe;
	
err2:
	kfree(state);
err1:
	return NULL;
}
EXPORT_SYMBOL_GPL(mxl603_attach);

MODULE_DESCRIPTION("MaxLinear MXL603 tuner driver");
MODULE_AUTHOR("Xiaodong Ni <nxiaodong520@gmail.com>");
MODULE_LICENSE("GPL");
