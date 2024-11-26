#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
#include <dvb_frontend.h>
#else
#include <media/dvb_frontend.h>
#endif
#include "mxl603_api.h"

/* MxLWare Driver version for MxL603 */
UINT8 MxLWare603DrvVersion[] = {2, 1, 1, 3, 0}; 

MXL603_REG_CTRL_INFO_T MxL603_OverwriteDefaults[] = 
{
  {0x14, 0xFF, 0x13},
  {0x6D, 0xFF, 0x8A},
  {0x6D, 0xFF, 0x0A},
  {0xDF, 0xFF, 0x19},
  {0x45, 0xFF, 0x1B},
  {0xA9, 0xFF, 0x59},
  {0xAA, 0xFF, 0x6A},
  {0xBE, 0xFF, 0x4C},
  {0xCF, 0xFF, 0x25},
  {0xD0, 0xFF, 0x34},
  {0x77, 0xFF, 0xE7},
  {0x78, 0xFF, 0xE3},
  {0x6F, 0xFF, 0x51},
  {0x7B, 0xFF, 0x84},
  {0x7C, 0xFF, 0x9F},
  {0x56, 0xFF, 0x41},
  {0xCD, 0xFF, 0x64},
  {0xC3, 0xFF, 0x2C},
  {0x9D, 0xFF, 0x61},
  {0xF7, 0xFF, 0x52},
  {0x58, 0xFF, 0x81},
  {0x00, 0xFF, 0x01},
  {0x62, 0xFF, 0x02},
  {0x00, 0xFF, 0x00},
  {0,    0,    0}
};

MXL603_REG_CTRL_INFO_T MxL603_DigitalIsdbtAtsc[] = 
{
  {0x0C, 0xFF, 0x00},
  {0x13, 0xFF, 0x04},
  {0x53, 0xFF, 0xFE},
  {0x57, 0xFF, 0x91},
  {0x62, 0xFF, 0xC2},
  {0x6E, 0xFF, 0x01},
  {0x6F, 0xFF, 0x51},
  {0x87, 0xFF, 0x77},
  {0x88, 0xFF, 0x55},
  {0x93, 0xFF, 0x22},
  {0x97, 0xFF, 0x02},
  {0xBA, 0xFF, 0x30},
  {0x98, 0xFF, 0xAF},
  {0x9B, 0xFF, 0x20},
  {0x9C, 0xFF, 0x1E},
  {0xA0, 0xFF, 0x18},
  {0xA5, 0xFF, 0x09},
  {0xC2, 0xFF, 0xA9},
  {0xC5, 0xFF, 0x7C},
  {0xCD, 0xFF, 0xEB},
  {0xCE, 0xFF, 0x7F},
  {0xD5, 0xFF, 0x03},
  {0xD9, 0xFF, 0x04},
  {0,    0,    0}
};
// Digital DVB-C application mode setting
MXL603_REG_CTRL_INFO_T MxL603_DigitalDvbc[] = 
{
  {0x0C, 0xFF, 0x00},
  {0x13, 0xFF, 0x04},
  {0x53, 0xFF, 0x7E},
  {0x57, 0xFF, 0x91},
  {0x5C, 0xFF, 0xB1},
  {0x62, 0xFF, 0xF2},
  {0x6E, 0xFF, 0x03},
  {0x6F, 0xFF, 0xD1},
  {0x87, 0xFF, 0x77},
  {0x88, 0xFF, 0x55},
  {0x93, 0xFF, 0x33},
  {0x97, 0xFF, 0x03},
  {0xBA, 0xFF, 0x40},
  {0x98, 0xFF, 0xAF},
  {0x9B, 0xFF, 0x20},
  {0x9C, 0xFF, 0x1E},
  {0xA0, 0xFF, 0x18},
  {0xA5, 0xFF, 0x09},
  {0xC2, 0xFF, 0xA9},
  {0xC5, 0xFF, 0x7C},
  {0xCD, 0xFF, 0x64},
  {0xCE, 0xFF, 0x7C},
  {0xD5, 0xFF, 0x05},
  {0xD9, 0xFF, 0x00},
  {0xEA, 0xFF, 0x00},
  {0xDC, 0xFF, 0x1C},
  {0,    0,    0}
};
// Digital DVB-T 6MHz application mode setting
 MXL603_REG_CTRL_INFO_T MxL603_DigitalDvbt[] = 
{
  {0x0C, 0xFF, 0x00},
  {0x13, 0xFF, 0x04},
  {0x53, 0xFF, 0xFE},
  {0x57, 0xFF, 0x91},
  {0x62, 0xFF, 0xC2},
  {0x6E, 0xFF, 0x01},
  {0x6F, 0xFF, 0x51},
  {0x87, 0xFF, 0x77},
  {0x88, 0xFF, 0x55},
  {0x93, 0xFF, 0x22},
  {0x97, 0xFF, 0x02},
  {0xBA, 0xFF, 0x30},
  {0x98, 0xFF, 0xAF},
  {0x9B, 0xFF, 0x20},
  {0x9C, 0xFF, 0x1E},
  {0xA0, 0xFF, 0x18},
  {0xA5, 0xFF, 0x09},
  {0xC2, 0xFF, 0xA9},
  {0xC5, 0xFF, 0x7C},
  {0xCD, 0xFF, 0x64},
  {0xCE, 0xFF, 0x7C},
  {0xD5, 0xFF, 0x03},
  {0xD9, 0xFF, 0x04},
  {0,    0,    0}
};

int MXL603_Write(struct i2c_adapter *i2c, u8 tuner_addr, UINT8 RegAddr, UINT8 RegData)
{
	int ret;

	u8 buf1[]={RegAddr,RegData} ;
	

	struct i2c_msg msg= {
		.addr= tuner_addr,
		.flags=0,
		.buf=buf1,
		.len=2
		};


	ret = i2c_transfer(i2c, &msg, 1);

	if (ret != 1)
		printk(" writereg error (reg == 0x%02x, val == 0x%02x, "
			"ret == %i)\n",RegAddr, RegData, ret);

	return (ret != 1) ? 1 : 0;


//	return 0;

	
}

int MXL603_Read(struct i2c_adapter *i2c, u8 tuner_addr, UINT8 RegAddr , UINT8 *rdata )
{
	int ret;
	u8 b0[2] = { 0xFB ,RegAddr};

/*	struct i2c_msg msg[] = {
		{
			.addr = tuner_addr,
			.flags = 0,
			.buf = b0,
			.len = 2
		}, {
			.addr = tuner_addr,
			.flags = I2C_M_RD,
			.buf = rdata,
			.len = 1
		}
	};*/
	struct i2c_msg msg[] = {
		{
			.addr = tuner_addr,
			.flags = I2C_M_RD,
			.buf = rdata,
			.len = 1
		}
	};

	ret = MXL603_Write(i2c, tuner_addr, 0xfb, RegAddr);
	ret |= i2c_transfer(i2c, msg, 1);

	if (ret != 1)
		printk(" readreg error (reg == 0x%02x, ret == %i)\n",
				 RegAddr, ret);


	return (ret != 1) ? 1 : 0;
//	return 0;
	
}

MXL_STATUS MxLWare603_OEM_WriteRegister(struct i2c_adapter *i2c, u8 tuner_addr, UINT8 RegAddr, UINT8 RegData)
{
	int Status = 0;

	Status = MXL603_Write(i2c, tuner_addr, RegAddr, RegData);


	if ( Status != 0 )
	{

		printk("MxLWare603_OEM_WriteRegister error!!");

	}

	return (MXL_STATUS)Status;
}


MXL_STATUS MxLWare603_OEM_ReadRegister(struct i2c_adapter *i2c, u8 tuner_addr, UINT8 RegAddr, UINT8 *DataPtr)
{
	int Status = 0;

	Status = MXL603_Read(i2c, tuner_addr, RegAddr, DataPtr);


	if ( Status != 0 )
	{
		printk("MxLWare603_OEM_ReadRegister error!!");
	}

//	printk("read reg=%x val=%x\n", RegAddr, *DataPtr); 

	return ( MXL_STATUS)Status;
}

MXL_STATUS MxL603_Ctrl_ProgramRegisters(struct i2c_adapter *i2c, u8 tuner_addr,  PMXL603_REG_CTRL_INFO_T ctrlRegInfoPtr)
{
	MXL_STATUS status = MXL_TRUE;
	UINT16 i = 0;
	UINT8 tmp = 0;

	while (status == MXL_TRUE)
	{
		if ((ctrlRegInfoPtr[i].regAddr == 0) && (ctrlRegInfoPtr[i].mask == 0) && (ctrlRegInfoPtr[i].data == 0)) break;

		// Check if partial bits of register were updated
		if (ctrlRegInfoPtr[i].mask != 0xFF)  
		{
			status = MxLWare603_OEM_ReadRegister(i2c, tuner_addr, ctrlRegInfoPtr[i].regAddr, &tmp);
			if (status != MXL_TRUE) break;;
		}

		tmp &= (UINT8) ~ctrlRegInfoPtr[i].mask;
		tmp |= (UINT8) ctrlRegInfoPtr[i].data;

		status = MxLWare603_OEM_WriteRegister(i2c, tuner_addr, ctrlRegInfoPtr[i].regAddr, tmp);
		if (status != MXL_TRUE) break;

		i++;
	}

	return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgDevSoftReset
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API is used to reset MxL603 tuner device. After reset,
--|                 all the device regiaters and modules will be set to power-on  
--|                 default state. 
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_FAILED 
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgDevSoftReset(struct i2c_adapter *i2c, u8 tuner_addr)
{
	UINT8 status = MXL_SUCCESS;

	//  MxL_DLL_DEBUG0("%s", __FUNCTION__); 

	// Write 0xFF with 0 to reset tuner 
	status = MxLWare603_OEM_WriteRegister(i2c, tuner_addr, AIC_RESET_REG, 0x00); 

	return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgDevOverwriteDefaults
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : Register(s) that requires default values to be overwritten 
--|                 during initialization
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgDevOverwriteDefaults(struct i2c_adapter *i2c, u8 tuner_addr, 
															 MXL_BOOL singleSupply_3_3V)
{
	UINT8 status = MXL_SUCCESS;
	UINT8 readData = 0;

	//  MxL_DLL_DEBUG0("%s", __FUNCTION__); 

	status |= MxL603_Ctrl_ProgramRegisters(i2c, tuner_addr, MxL603_OverwriteDefaults);

	status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x00, 0x01);
	status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, 0x31, &readData);
	readData &= 0x2F;
	readData |= 0xD0;
	status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x31, readData);
	status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x00, 0x00);


	/* If Single supply 3.3v is used */
	if (MXL_ENABLE == singleSupply_3_3V)
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, MAIN_REG_AMP, 0x04);

	return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgDevXtal
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API is used to configure XTAL settings of MxL603 tuner
--|                 device. XTAL settings include frequency, capacitance & 
--|                 clock out
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgDevXtal(struct i2c_adapter *i2c, u8 tuner_addr,  MXL603_XTAL_SET_CFG_T xtalCfg)
{
	UINT8 status = MXL_SUCCESS;
	UINT8 control = 0;

	// MxL_DLL_DEBUG0("%s", __FUNCTION__); 

	// XTAL freq and cap setting, Freq set is located at bit<5>, cap bit<4:0> 
	// and  XTAL clock out enable <0>
	if ((xtalCfg.xtalFreqSel == MXL603_XTAL_16MHz) || (xtalCfg.xtalFreqSel == MXL603_XTAL_24MHz))
	{
		control = (UINT8)((xtalCfg.xtalFreqSel << 5) | (xtalCfg.xtalCap & 0x1F));  
		control |= (xtalCfg.clkOutEnable << 7);
		status = MxLWare603_OEM_WriteRegister(i2c, tuner_addr, XTAL_CAP_CTRL_REG, control);

		// XTAL frequency div 4 setting <1> 
		control = (0x01 & (UINT8)xtalCfg.clkOutDiv);

		// XTAL sharing mode
		if (xtalCfg.XtalSharingMode == MXL_ENABLE) control |= 0x40;
		else control &= 0x01;

		// program Clock out div & Xtal sharing
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, XTAL_ENABLE_DIV_REG, control); 

		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x6d, 0x0a); 			//runqida

		// Main regulator re-program
		if (MXL_ENABLE == xtalCfg.singleSupply_3_3V)
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, MAIN_REG_AMP, 0x14);
	}
	else 
		status |= MXL_INVALID_PARAMETER;

	return ( MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgDevPowerMode
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function configures MxL603 power mode 
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgDevPowerMode(struct i2c_adapter *i2c, u8 tuner_addr, MXL603_PWR_MODE_E powerMode)
{
	UINT8 status = MXL_SUCCESS;

	// MxL_DLL_DEBUG0("%s", __FUNCTION__); 

	switch(powerMode)
	{
	case MXL603_PWR_MODE_SLEEP:
		break;

	case MXL603_PWR_MODE_ACTIVE:
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, TUNER_ENABLE_REG, MXL_ENABLE);
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, START_TUNE_REG, MXL_ENABLE);
		break;

	case MXL603_PWR_MODE_STANDBY:
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, START_TUNE_REG, MXL_DISABLE);
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, TUNER_ENABLE_REG, MXL_DISABLE);
		break;

	default:
		status |= MXL_INVALID_PARAMETER;
	}

	//runqida
	status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, PAGE_CHANGE_REG, 0x01);	
	status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x60, 0x37);	
	status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, PAGE_CHANGE_REG, 0x00);

	return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgDevGPO
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API configures GPO pin of MxL603 tuner device.
--|                 There is only 1 GPO pin available in MxL603 device.  
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgDevGPO(struct i2c_adapter *i2c, u8 tuner_addr, MXL603_GPO_STATE_E gpoState)
{
	UINT8 status = MXL_SUCCESS;
	UINT8 regData = 0;

	//  MxL_DLL_DEBUG0("%s", __FUNCTION__); 

	switch(gpoState)
	{
	case MXL603_GPO_AUTO_CTRL:
	case MXL603_GPO_HIGH:
	case MXL603_GPO_LOW:
		status = MxLWare603_OEM_ReadRegister(i2c, tuner_addr, GPO_SETTING_REG, &regData);
		if (MXL603_GPO_AUTO_CTRL == gpoState)
			regData &= 0xFE;
		else
		{
			regData &= 0xFC;
			regData |= (UINT8)(0x01 | (gpoState << 1)); 
		}

		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, GPO_SETTING_REG, regData);
		break;

	default:
		status = MXL_INVALID_PARAMETER;
	}

	return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_ReqDevVersionInfo
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function is used to get MxL603 version information.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_ReqDevVersionInfo(struct i2c_adapter *i2c, u8 tuner_addr, MXL603_VER_INFO_T* mxlDevVerInfoPtr)
														
{
	UINT8 status = MXL_SUCCESS;
	UINT8 readBack = 0;
	UINT8 k = 0;


	// MxL_DLL_DEBUG0("%s", __FUNCTION__); 

	if (mxlDevVerInfoPtr)
	{
		status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, CHIP_ID_REQ_REG, &readBack);
		mxlDevVerInfoPtr->chipId = (readBack & 0xFF); 

		status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, CHIP_VERSION_REQ_REG, &readBack);
		mxlDevVerInfoPtr->chipVersion = (readBack & 0xFF); 

	//	printk("Chip ID = 0x%d, Version = 0x%d \n", mxlDevVerInfoPtr->chipId, 
	//		mxlDevVerInfoPtr->chipVersion);

		// Get MxLWare version infromation
		for (k = 0; k < MXL603_VERSION_SIZE; k++)
			mxlDevVerInfoPtr->mxlwareVer[k] = MxLWare603DrvVersion[k];
	}
	else 
		status = MXL_INVALID_PARAMETER;

	return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_ReqDevGPOStatus
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API is used to get GPO pin's status information from
--|                 MxL603 tuner device.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_ReqDevGPOStatus(struct i2c_adapter *i2c, u8 tuner_addr, 
													  MXL603_GPO_STATE_E* gpoStatusPtr)
{
	UINT8 status = MXL_SUCCESS;
	UINT8 regData = 0;

	//MxL_DLL_DEBUG0("%s", __FUNCTION__); 

	if (gpoStatusPtr)
	{
		status = MxLWare603_OEM_ReadRegister(i2c, tuner_addr, GPO_SETTING_REG, &regData);

		// GPO1 bit<1:0>
		if ((regData & 0x01) == 0) *gpoStatusPtr = MXL603_GPO_AUTO_CTRL;
		else *gpoStatusPtr = (MXL603_GPO_STATE_E)((regData & 0x02) >> 1);
	}
	else
		status = MXL_INVALID_PARAMETER;

	return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgTunerMode
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This fucntion is used to configure MxL603 tuner's 
--|                 application modes like DVB-T, DVB-C, ISDB-T etc.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgTunerMode(struct i2c_adapter *i2c, u8 tuner_addr, MXL603_TUNER_MODE_CFG_T tunerModeCfg)
												   
{
	UINT8 status = MXL_SUCCESS;
	UINT8 dfeRegData = 0;
	//struct MXL603_REG_CTRL_INFO_T* tmpRegTable;

////	printk(" Signal Mode = %d, IF Freq = %d, xtal = %d, IF Gain = %d", 
//		tunerModeCfg.signalMode,
//		tunerModeCfg.ifOutFreqinKHz,
//		tunerModeCfg.xtalFreqSel,
//		tunerModeCfg.ifOutGainLevel); 

	switch(tunerModeCfg.signalMode)
	{
	case MXL603_DIG_DVB_C:
	case MXL603_DIG_J83B:
	//	tmpRegTable = MxL603_DigitalDvbc;
		status = MxL603_Ctrl_ProgramRegisters(i2c, tuner_addr, MxL603_DigitalDvbc);

		if (tunerModeCfg.ifOutFreqinKHz < HIGH_IF_35250_KHZ)
		{
			// Low power
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_0, 0xFE);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_1, 0x10);
		}
		else
		{
			// High power
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_0, 0xD9);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_1, 0x16);
		}

		if (tunerModeCfg.xtalFreqSel == MXL603_XTAL_16MHz) dfeRegData = 0x0D;
		else if (tunerModeCfg.xtalFreqSel == MXL603_XTAL_24MHz) dfeRegData = 0x0E;
		else status |= MXL_INVALID_PARAMETER;

		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DFE_CSF_SS_SEL, dfeRegData);

		break;

	case MXL603_DIG_ISDBT_ATSC:
		//tmpRegTable = MxL603_DigitalIsdbtAtsc;
		status = MxL603_Ctrl_ProgramRegisters(i2c, tuner_addr, MxL603_DigitalIsdbtAtsc);

		if (tunerModeCfg.ifOutFreqinKHz < HIGH_IF_35250_KHZ)
		{
			// Low power
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_0, 0xF9);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_1, 0x18);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_PWR, 0xF1);
		}
		else
		{
			// High power
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_0, 0xD9);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_1, 0x16);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_PWR, 0xB1);
		}

		if (MXL603_XTAL_16MHz == tunerModeCfg.xtalFreqSel) dfeRegData = 0x0D;
		else if (MXL603_XTAL_24MHz == tunerModeCfg.xtalFreqSel) dfeRegData = 0x0E;
		else status |= MXL_INVALID_PARAMETER;

		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DFE_CSF_SS_SEL, dfeRegData);

		dfeRegData = 0;
		switch(tunerModeCfg.ifOutGainLevel)
		{
		case 0x09: dfeRegData = 0x44; break;
		case 0x08: dfeRegData = 0x43; break;
		case 0x07: dfeRegData = 0x42; break;
		case 0x06: dfeRegData = 0x41; break;
		case 0x05: dfeRegData = 0x40; break;
		default: break;
		}
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DFE_DACIF_GAIN, dfeRegData);

		break;

	case MXL603_DIG_DVB_T_DTMB:
		//tmpRegTable = MxL603_DigitalDvbt;
		status = MxL603_Ctrl_ProgramRegisters(i2c, tuner_addr, MxL603_DigitalDvbt);

		if (tunerModeCfg.ifOutFreqinKHz < HIGH_IF_35250_KHZ)
		{
			// Low power
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_0, 0xFE);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_1, 0x18);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_PWR, 0xF1);
		}
		else
		{
			// High power
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_0, 0xD9);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_CFG_1, 0x16);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_IF_PWR, 0xB1);
		}

		if (MXL603_XTAL_16MHz == tunerModeCfg.xtalFreqSel) dfeRegData = 0x0D;
		else if (MXL603_XTAL_24MHz == tunerModeCfg.xtalFreqSel) dfeRegData = 0x0E;
		else status |= MXL_INVALID_PARAMETER;

		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DFE_CSF_SS_SEL, dfeRegData);

		dfeRegData = 0;
		switch(tunerModeCfg.ifOutGainLevel)
		{
		case 0x0B: dfeRegData = 0x47; break;	//check
		case 0x09: dfeRegData = 0x44; break;
		case 0x08: dfeRegData = 0x43; break;
		case 0x07: dfeRegData = 0x42; break;
		case 0x06: dfeRegData = 0x41; break;
		case 0x05: dfeRegData = 0x40; break;
		default: break;
		}
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DFE_DACIF_GAIN, dfeRegData);
		break;

	default:
		status = MXL_INVALID_PARAMETER;
		break;
	}

	if (status == MXL_SUCCESS)  
	{
		// XTAL calibration
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, XTAL_CALI_SET_REG, 0x00);   
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, XTAL_CALI_SET_REG, 0x01);   

		// 50 ms sleep after XTAL calibration
		msleep(50);
	}

	return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgTunerAGC
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function is used to configure AGC settings of MxL603
--|                 tuner device.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgTunerAGC(struct i2c_adapter *i2c, u8 tuner_addr, MXL603_AGC_CFG_T agcCfg)
{
	UINT8 status = MXL_SUCCESS;
	UINT8 regData = 0; 

//	printk("%s, AGC sel = %d, attack point set = %d, Flip = %d \n", 
//		__FUNCTION__, 
//		agcCfg.agcType,
//		agcCfg.setPoint, 
//		agcCfg.agcPolarityInverstion);

	if ((agcCfg.agcPolarityInverstion <= MXL_ENABLE) && 
		(agcCfg.agcType <= MXL603_AGC_EXTERNAL))
	{
		// AGC selecton <3:2> and mode setting <0>
		status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, AGC_CONFIG_REG, &regData); 
		regData &= 0xF2; // Clear bits <3:2> & <0>
		regData = (UINT8) (regData | (agcCfg.agcType << 2) | 0x01);
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, AGC_CONFIG_REG, regData);

		// AGC set point <6:0>
		status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, AGC_SET_POINT_REG, &regData);
		regData &= 0x80; // Clear bit <6:0>
		regData |= agcCfg.setPoint;
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, AGC_SET_POINT_REG, regData);

		// AGC Polarity <4>
		status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, AGC_FLIP_REG, &regData);
		regData &= 0xEF; // Clear bit <4>
		regData |= (agcCfg.agcPolarityInverstion << 4);
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, AGC_FLIP_REG, regData);
	}
	else
		status = MXL_INVALID_PARAMETER;

	return(MXL_STATUS) status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgTunerLoopThrough
--| 
--| AUTHOR        : Mahendra Kondur, Dong Liu
--|
--| DATE CREATED  : 12/10/2011, 06/18/2012   
--|
--| DESCRIPTION   : This function is used to enable or disable Loop-Through
--|                 settings of MxL603 tuner device.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgTunerLoopThrough(struct i2c_adapter *i2c, u8 tuner_addr, MXL_BOOL loopThroughCtrl)
{
	UINT8 status = MXL_SUCCESS, regData;

//	MxL_DLL_DEBUG0("%s", __FUNCTION__); 

	if (loopThroughCtrl <= MXL_ENABLE)
	{
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, PAGE_CHANGE_REG, 0x01);

		status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, DIG_ANA_GINJO_LT_REG, &regData);

		if (loopThroughCtrl == MXL_ENABLE)
			regData |= 0x10;  // Bit<4> = 1       
		else
			regData &= 0xEF;  // Bit<4> = 0  
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DIG_ANA_GINJO_LT_REG, regData);

		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, PAGE_CHANGE_REG, 0x00);
	}
	else
		status = MXL_INVALID_PARAMETER;

	return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgTunerChanTune
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API configures RF channel frequency and bandwidth. 
--|                 Radio Frequency unit is Hz, and Bandwidth is in MHz units.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgTunerChanTune(struct i2c_adapter *i2c, u8 tuner_addr, MXL603_CHAN_TUNE_CFG_T chanTuneCfg)
													   
{
	UINT64 frequency;
	UINT32 freq = 0;
	UINT8 status = MXL_SUCCESS;
	UINT8 regData = 0;
	UINT8 agcData = 0;
	UINT8 dfeTuneData = 0;
	UINT8 dfeCdcData = 0;

//	printk("%s, signal type = %d, Freq = %d, BW = %d, Xtal = %d \n",  
//		__FUNCTION__,
//		chanTuneCfg.signalMode, 
//		chanTuneCfg.freqInHz, 
//		chanTuneCfg.bandWidth, 
//		chanTuneCfg.xtalFreqSel);

	// Abort Tune
	status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, START_TUNE_REG, 0x00); 

	if (chanTuneCfg.startTune == MXL_ENABLE)
	{
		if (chanTuneCfg.signalMode <= MXL603_DIG_J83B) 
		{
			// RF Frequency VCO Band Settings 
			if (chanTuneCfg.freqInHz < 700000000) 
			{
				status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x7C, 0x1F);
				if ((chanTuneCfg.signalMode == MXL603_DIG_DVB_C) || (chanTuneCfg.signalMode == MXL603_DIG_J83B)) 
					regData = 0xC1;
				else
					regData = 0x81;

			}
			else 
			{
				status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x7C, 0x9F);
				if ((chanTuneCfg.signalMode == MXL603_DIG_DVB_C) || (chanTuneCfg.signalMode == MXL603_DIG_J83B)) 
					regData = 0xD1;
				else
					regData = 0x91;

			}

			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x00, 0x01);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x31, regData);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x00, 0x00);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DFE_REFLUT_BYP_REG, 0x00);		//runqida
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, DFE_REFSX_INT_MOD_REG, 0xd8);	//runqida

			// Bandwidth <7:0>
			switch(chanTuneCfg.bandWidth)
			{
			case MXL603_CABLE_BW_6MHz:
			case MXL603_CABLE_BW_7MHz:
			case MXL603_CABLE_BW_8MHz:
			case MXL603_TERR_BW_6MHz:
			case MXL603_TERR_BW_7MHz:
			case MXL603_TERR_BW_8MHz:
				status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, CHAN_TUNE_BW_REG, (UINT8)chanTuneCfg.bandWidth);

				// Frequency
				frequency = chanTuneCfg.freqInHz;

				/* Calculate RF Channel = DIV(64*RF(Hz), 1E6) */
				frequency *= 64;
				do_div(frequency, 1000000);
				freq = (UINT32)frequency;
//				freq = (UINT32)(frequency / 1000000); 

				// Set RF  
				status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, CHAN_TUNE_LOW_REG, (UINT8)(freq & 0xFF));
				status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, CHAN_TUNE_HI_REG, (UINT8)((freq >> 8 ) & 0xFF));
				break;

			default:
				status |= MXL_INVALID_PARAMETER;
				break;
			}

			// Power up tuner module
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, TUNER_ENABLE_REG, 0x01);

			// Start Sequencer settings
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, PAGE_CHANGE_REG, 0x01); 
			status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, DIG_ANA_GINJO_LT_REG, &regData);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, PAGE_CHANGE_REG, 0x00); 

			status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, 0xB6, &agcData);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, PAGE_CHANGE_REG, 0x01); 
			status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, 0x60, &dfeTuneData);
			status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, 0x5F, &dfeCdcData);

			// Check if LT is enabled
			if ((regData & 0x10) == 0x10)
			{
				// dfe_agc_auto = 0 & dfe_agc_rf_bo_w = 14
				agcData &= 0xBF;
				agcData |= 0x0E;

				// dfe_seq_tune_rf1_bo = 14
				dfeTuneData &= 0xC0;
				dfeTuneData |= 0x0E;

				// dfe_seq_cdc_rf1_bo = 14
				dfeCdcData &= 0xC0;
				dfeCdcData |= 0x0E;
			}
			else
			{
				// dfe_agc_auto = 1 & dfe_agc_rf_bo_w = 0
				agcData |= 0x40;
				agcData &= 0xC0;

				// dfe_seq_tune_rf1_bo = 55
				dfeTuneData &= 0xC0;
				dfeTuneData |= 0x37;

				// dfe_seq_cdc_rf1_bo = 55
				dfeCdcData &= 0xC0;
				dfeCdcData |= 0x37;
			}

			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x60, dfeTuneData); 
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x5F, dfeCdcData); 
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, PAGE_CHANGE_REG, 0x00); 
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0xB6, agcData); 

			// Bit <0> 1 : start , 0 : abort calibrations
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, START_TUNE_REG, 0x01); 

			// Sleep 15 ms
			msleep(15);

			// dfe_agc_auto = 1 
			agcData = (agcData | 0x40);
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0xB6, agcData); 

		}
		else
			status = MXL_INVALID_PARAMETER;
	}

	return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_CfgTunerIFOutParam
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function is used to configure IF out settings of MxL603 
--|                 tuner device.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_CfgTunerIFOutParam(struct i2c_adapter *i2c, u8 tuner_addr,  MXL603_IF_OUT_CFG_T ifOutCfg)
{
	UINT16 ifFcw;
	UINT8 status = MXL_SUCCESS;
	UINT8 readData = 0;
	UINT8 control = 0;
	UINT64 tmp;

//	printk("%s, Manual set = %d \n", __FUNCTION__, ifOutCfg.manualFreqSet); 

	//Test only 
//	MxLWare603_OEM_WriteRegister(i2c, tuner_addr, 0x10, 0x99);
//	MxLWare603_OEM_ReadRegister(i2c, tuner_addr, 0x10, &readData);
//	printk("\n ----------- test Tuner I2C read out = 0x%x [ if 0x99, I2C OK!]-------------\n", readData); 

	// Read back register for manual IF Out 
	status = MxLWare603_OEM_ReadRegister(i2c, tuner_addr, IF_FREQ_SEL_REG, &readData);

	if (ifOutCfg.manualFreqSet == MXL_ENABLE)
	{
//		printk("%s, IF Freq = %d \n", __FUNCTION__, ifOutCfg.manualIFOutFreqInKHz); 

		// IF out manual setting : bit<5>
		readData |= 0x20;
		status = MxLWare603_OEM_WriteRegister(i2c, tuner_addr, IF_FREQ_SEL_REG, readData);

		// Manual IF freq set
//		tmp = ifOutCfg.manualIFOutFreqInKHz * 8192;
//		do_div(tmp, 216000);
//		ifFcw = (UINT16)tmp;
		ifFcw = (UINT16)(ifOutCfg.manualIFOutFreqInKHz * 8192 / 216000);
		control = (ifFcw & 0xFF); // Get low 8 bit 
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, IF_FCW_LOW_REG, control); 

		control = ((ifFcw >> 8) & 0x0F); // Get high 4 bit 
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, IF_FCW_HIGH_REG, control);
	}
	else if (ifOutCfg.manualFreqSet == MXL_DISABLE)
	{
		// bit<5> = 0, use IF frequency from IF frequency table  
		readData &= 0xC0;

		// IF Freq <4:0>
		readData |= ifOutCfg.ifOutFreq;
		status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, IF_FREQ_SEL_REG, readData);
	}
	else
		status |= MXL_INVALID_PARAMETER;

	if (status == MXL_SUCCESS)
	{
		// Set spectrum invert, gain level and IF path 
		// Spectrum invert indication is bit<7:6>
		if (ifOutCfg.ifInversion <= MXL_ENABLE)
		{
			control = 0;
			if (MXL_ENABLE == ifOutCfg.ifInversion) control = 0x3 << 6;

			// Gain level is bit<3:0> 
			control += (ifOutCfg.gainLevel & 0x0F);
			control |= (0x20); // Enable IF out
			status |= MxLWare603_OEM_WriteRegister(i2c, tuner_addr, IF_PATH_GAIN_REG, control);
		} 
		else
			status |= MXL_INVALID_PARAMETER;
	}

	return(MXL_STATUS) status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_ReqTunerAGCLock
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function returns AGC Lock status of MxL603 tuner.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_ReqTunerAGCLock(struct i2c_adapter *i2c, u8 tuner_addr, MXL_BOOL* agcLockStatusPtr)
{
	 MXL_STATUS status = MXL_SUCCESS;
	UINT8 regData = 0;
	 MXL_BOOL lockStatus = MXL_UNLOCKED;

	if (agcLockStatusPtr)
	{
		status = MxLWare603_OEM_ReadRegister(i2c, tuner_addr, AGC_SAGCLOCK_STATUS_REG, &regData);  
		if ((regData & 0x08) == 0x08) lockStatus = MXL_LOCKED;

		*agcLockStatusPtr =  lockStatus;

		//printk(" Agc lock = %d", (UINT8)*agcLockStatusPtr); 
	}
	else
		status = MXL_INVALID_PARAMETER;

	return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_ReqTunerLockStatus
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function returns Tuner Lock status of MxL603 tuner.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_ReqTunerLockStatus(struct i2c_adapter *i2c, u8 tuner_addr,  MXL_BOOL* rfLockPtr, 
														 	 MXL_BOOL* refLockPtr)
{
	MXL_STATUS status = MXL_SUCCESS;
	UINT8 regData = 0;
	MXL_BOOL rfLockStatus = MXL_UNLOCKED;
	MXL_BOOL refLockStatus = MXL_UNLOCKED;

	//  MxL_DLL_DEBUG0("%s", __FUNCTION__);

	if ((rfLockPtr) && (refLockPtr))
	{
		status = MxLWare603_OEM_ReadRegister(i2c, tuner_addr, RF_REF_STATUS_REG, &regData);  

		if ((regData & 0x02) == 0x02) rfLockStatus = MXL_LOCKED;
		if ((regData & 0x01) == 0x01) refLockStatus = MXL_LOCKED;

		*rfLockPtr =  rfLockStatus;
		*refLockPtr = refLockStatus;
	}
	else
		status = MXL_INVALID_PARAMETER;

	return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_API_ReqTunerRxPower
--| 
--| AUTHOR        : Mahendra Kondur
--|                 Dong Liu 
--|
--| DATE CREATED  : 12/10/2011
--|                 06/18/2012
--|
--| DESCRIPTION   : This function returns RF input power in 0.01dBm.
--|
--| RETURN VALUE  : MXL_SUCCESS, MXL_INVALID_PARAMETER, MXL_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_API_ReqTunerRxPower(struct i2c_adapter *i2c, u8 tuner_addr, SINT16* rxPwrPtr)
{
	UINT8 status = MXL_SUCCESS;
	UINT8 regData = 0;
	UINT16 tmpData = 0;

	//  MxL_DLL_DEBUG0("%s", __FUNCTION__);

	if (rxPwrPtr)
	{
		// RF input power low <7:0>
		status = MxLWare603_OEM_ReadRegister(i2c, tuner_addr, RFPIN_RB_LOW_REG, &regData);
		tmpData = regData;

		// RF input power high <1:0>
		status |= MxLWare603_OEM_ReadRegister(i2c, tuner_addr, RFPIN_RB_HIGH_REG, &regData);
		tmpData |= (regData & 0x03) << 8;

		// Fractional last 2 bits
		*rxPwrPtr = (tmpData & 0x01FF) * 25;  //100 times dBm

		if (tmpData & 0x02) *rxPwrPtr += 50;;
		if (tmpData & 0x01) *rxPwrPtr += 25;
		if (tmpData & 0x0200) *rxPwrPtr -= 128*100;

//		printk(" Rx power = %d times of 0.01dBm \n", *rxPwrPtr);
	}
	else
		status = MXL_INVALID_PARAMETER;

	return (MXL_STATUS)status;
}


MXL_STATUS MXL603_init(struct i2c_adapter *i2c, u8 tuner_addr, struct mxl603_config cfg)
{
	MXL_STATUS status; 
//	UINT8 devId;
/*	MXL_BOOL singleSupply_3_3V;
	MXL603_XTAL_SET_CFG_T xtalCfg;
	MXL603_IF_OUT_CFG_T ifOutCfg;
	MXL603_AGC_CFG_T agcCfg;
	MXL603_TUNER_MODE_CFG_T tunerModeCfg;*/
	MXL603_VER_INFO_T	mxl603Version;
	
	MxLWare603_API_ReqDevVersionInfo(i2c, tuner_addr, &mxl603Version);


	/* If OEM data is not required, customer should treat devId as 
	I2C slave Address */
//	devId = MXL603_I2C_ADDR;

	//Step 1 : Soft Reset MxL603
	status = MxLWare603_API_CfgDevSoftReset(i2c, tuner_addr);
	if (status != MXL_SUCCESS)
	{
		printk("Error! MxLWare603_API_CfgDevSoftReset\n");    
	}

	//Step 2 : Overwrite Default
	/*Parameter : singleSupply_3_3V
	- Enable :  Single power supply to Tuner (3.3v only;  3.3v -> 1.8V tuner internally inverts ) 
	- Disable : Dual power supply to Tuner (3.3v+1.8v; internal regulator be bypassed) 
	\A1\EF If set wrongly toward hardware layout, Tuner will loose control of AGC(at least) 
	*/
//	singleSupply_3_3V = MXL_ENABLE; //MXL_DISABLE;
	status |= MxLWare603_API_CfgDevOverwriteDefaults(i2c, tuner_addr, cfg.singleSupply_3_3V);
	if (status != MXL_SUCCESS)
	{
		printk("Error! MxLWare603_API_CfgDevOverwriteDefaults\n");    
	}

	//Step 3 : XTAL Setting 

	/* Single XTAL for tuner and demod sharing*/
/*	xtalCfg.xtalFreqSel = MXL603_XTAL_24MHz;
	xtalCfg.xtalCap = 12; //Pls. set this based on the XTAL's SPEC : the matching capacitence to output accurate Clock
	xtalCfg.clkOutEnable = MXL_ENABLE;
	xtalCfg.clkOutDiv = MXL_DISABLE;
	xtalCfg.clkOutExt = MXL_DISABLE;
	xtalCfg.singleSupply_3_3V = MXL_ENABLE;
	xtalCfg.XtalSharingMode = MXL_DISABLE;*/
	status = MxLWare603_API_CfgDevXtal(i2c, tuner_addr, cfg.xtalCfg);
	if (status != MXL_SUCCESS)
	{
		printk("Error! MxLWare603_API_CfgDevXtal\n");    
	}

	status |= MxLWare603_API_CfgTunerLoopThrough(i2c, tuner_addr, MXL_DISABLE);
	if (status != MXL_SUCCESS)
	{
		printk("Error! MxLWare603_API_CfgTunerLoopThrough\n");    
	}
	
	//Step 4 : IF Out setting
	//IF freq set, should match Demod request 
/*	ifOutCfg.ifOutFreq = MXL603_IF_5MHz; //we suggest 5Mhz for ATSC MN88436 

	ifOutCfg.ifInversion = MXL_ENABLE;
	ifOutCfg.gainLevel = 11;
	ifOutCfg.manualFreqSet = MXL_ENABLE;
	ifOutCfg.manualIFOutFreqInKHz = 5000;*/
	status |= MxLWare603_API_CfgTunerIFOutParam(i2c, tuner_addr, cfg.ifOutCfg);
	if (status != MXL_SUCCESS)
	{
		printk("Error! MxLWare603_API_CfgTunerIFOutParam\n");    
	}

	//Step 5 : AGC Setting
	//agcCfg.agcType = MXL603_AGC_SELF; //if you doubt DMD IF-AGC part, pls. use Tuner self AGC instead.
/*	agcCfg.agcType = MXL603_AGC_EXTERNAL;
	agcCfg.setPoint = 66;
	agcCfg.agcPolarityInverstion = MXL_DISABLE;*/
	status |= MxLWare603_API_CfgTunerAGC(i2c, tuner_addr, cfg.agcCfg);
	if (status != MXL_SUCCESS)
	{
		printk("Error! MxLWare603_API_CfgTunerAGC\n");    
	}

	//Step 6 : Application Mode setting
//	tunerModeCfg.signalMode = MXL603_DIG_DVB_C;//MXL603_DIG_ISDBT_ATSC;//MXL603_DIG_DVB_T_DTMB;

	//IF freq set, should match Demod request 
//	tunerModeCfg.ifOutFreqinKHz = 5000;

	status |= MxLWare603_API_CfgDevPowerMode(i2c, tuner_addr, MXL603_PWR_MODE_ACTIVE);		//runqida

	/* Single XTAL for tuner and demod sharing*/
//	tunerModeCfg.xtalFreqSel = MXL603_XTAL_24MHz;
//	tunerModeCfg.ifOutGainLevel = 11;
	status = MxLWare603_API_CfgTunerMode(i2c, tuner_addr, cfg.tunerModeCfg);
	if (status != MXL_SUCCESS)
	{
		printk("##### Error! pls. make sure return value no problem, otherwise, it will cause Tuner unable to unlock signal #####\n");   
		printk("Error! MxLWare603_API_CfgTunerMode\n");    
	}




	// To Change Channel, GOTO Step #7

	// To change Application mode settings, GOTO Step #6

	return status;
}



MXL_STATUS Mxl603SetFreqBw(struct i2c_adapter *i2c, u8 tuner_addr, UINT32 freq, MXL603_BW_E bandWidth, MXL603_SIGNAL_MODE_E signalMode)
{
	MXL_STATUS status; 
//	UINT8 devId;
//	MXL_BOOL refLockPtr;
//	MXL_BOOL rfLockPtr;
	MXL603_CHAN_TUNE_CFG_T chanTuneCfg;
	UINT32 rf;
//	devId = MXL603_I2C_ADDR;
	
	//Step 7 : Channel frequency & bandwidth setting
//	printk("freq=%d",freq);
	if(freq>1000000)rf=freq;
	else if(freq>1000)rf=freq*1000;
	else
		rf=freq*1000000;
	
	
	chanTuneCfg.bandWidth = bandWidth;
	chanTuneCfg.freqInHz = rf;
//	if(param.system == DMD_E_ATSC)
//	chanTuneCfg.signalMode = MXL603_DIG_ISDBT_ATSC;
//	else
	chanTuneCfg.signalMode = signalMode;
	
	chanTuneCfg.startTune = MXL_START_TUNE;
	chanTuneCfg.xtalFreqSel =MXL603_XTAL_16MHz;

	//chanTuneCfgis global struct. 
	status = MxLWare603_API_CfgTunerChanTune(i2c, tuner_addr, chanTuneCfg);
	if (status != MXL_SUCCESS)
	{
		printk("Error! MxLWare603_API_CfgTunerChanTune\n");    
	}

	// Wait 15 ms 
/*	msleep(15);

	// Read back Tuner lock status
	status = MxLWare603_API_ReqTunerLockStatus(i2c, tuner_addr, &rfLockPtr, &refLockPtr);
	if (status == MXL_TRUE)
	{
		if (MXL_LOCKED == rfLockPtr && MXL_LOCKED == refLockPtr)
		{
			printk("Tuner locked\n"); //If system runs into here, it mean that Tuner locked and output IF OK!!
		}
		else
			printk("Tuner unlocked\n");
	}*/
	return status; 
}

