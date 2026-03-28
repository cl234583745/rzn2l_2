/*
* This source file is part of the EtherCAT Slave Stack Code licensed by Beckhoff Automation GmbH & Co KG, 33415 Verl, Germany.
* The corresponding license agreement applies. This hint shall not be removed.
*/

/**
\addtogroup SampleFoe FoE Sample functions
@{
*/

/**
\file samplefoe.c
\author EthercatSSC@beckhoff.com
\brief Implementation

*/


/*-----------------------------------------------------------------------------------------
------
------    Includes
------
-----------------------------------------------------------------------------------------*/
#include "applInterface.h"
#include "bootmode.h"
#define _FOE_SAMPLE_APPLICATION_
#include "samplefoe.h"
#undef _FOE_SAMPLE_APPLICATION_

#ifdef FW_PARSE_SREC
#include "r_fw_up_rz_if.h"
#endif
#include "flash_config.h"
#include "hal_data.h"
#include <stdio.h>
#include "log.h"

#define CURRENT_LOG_LEVEL   LOG_LEVEL_DEBUG

/*--------------------------------------------------------------------------------------
------
------    local types and defines
------
--------------------------------------------------------------------------------------*/
/*ECATCHANGE_START(V5.12) FOE1*/
#define    MAX_FILE_NAME_SIZE    32

/** \brief  MAX_FILE_SIZE: Maximum file size */
#define MAX_FILE_SIZE                             0x180
/*ECATCHANGE_END(V5.12) FOE1*/

/*-----------------------------------------------------------------------------------------
------
------    local variables and constants
------
-----------------------------------------------------------------------------------------*/
/*ECATCHANGE_START(V5.12) FOE1*/
UINT32           nFileWriteOffset;
CHAR             aFileName[MAX_FILE_NAME_SIZE];
UINT8 MBXMEM      aFileData[MAX_FILE_SIZE];


UINT32 u32FileSize;
UINT32 ReadDataAddr;	// Read data address for uploading
#if defined(BOARD_RZT2M_RSK)
    #if (BANK == 0)
    const UINT16 HUGE aFileNameHeader[8] = {0x4345,0x5441,0x5746,0x525F,0x545A,0x4D32,0x425F,0x3130}; // "ECATFW_RZT2M_B01"
    #elif (BANK == 1)
    const UINT16 HUGE aFileNameHeader[8] = {0x4345,0x5441,0x5746,0x525F,0x545A,0x4D32,0x425F,0x3030}; // "ECATFW_RZT2M_B00"
    #endif
#elif defined(BOARD_RZT2L_RSK)
    #if (BANK == 0)
    const UINT16 HUGE aFileNameHeader[8] = {0x4345,0x5441,0x5746,0x525F,0x545A,0x4C32,0x425F,0x3130}; // "ECATFW_RZT2L_B01"
    #elif (BANK == 1)
    const UINT16 HUGE aFileNameHeader[8] = {0x4345,0x5441,0x5746,0x525F,0x545A,0x4C32,0x425F,0x3030}; // "ECATFW_RZT2L_B00"
    #endif
#elif defined(BOARD_RZN2L_RSK)
    #if (BANK == 0)
//    const UINT16 HUGE aFileNameHeader[8] = {0x4345,0x5441,0x5746,0x525F,0x4E5A,0x4C32,0x425F,0x3130}; // "ECATFW_RZN2L_B01"
    const UINT16 HUGE aFileNameHeader[11] = {0x5a52,0x324e,0x5f4c,0x7378,0x6970,0x5f30,0x7061,0x3170,0x625f,0x6e61,0x306b};//RZN2L_xspi0_app1_bank0
    #elif (BANK == 1)
//    const UINT16 HUGE aFileNameHeader[8] = {0x4345,0x5441,0x5746,0x525F,0x4E5A,0x4C32,0x425F,0x3030}; // "ECATFW_RZN2L_B00"
    const UINT16 HUGE aFileNameHeader[11] = {0x525a,0x4e32,0x4c5f,0x7873,0x7069,0x305f,0x6170,0x7031,0x5f62,0x616e,0x6b31};//RZN2L_xspi0_app1_bank1
    #endif
#endif
const UINT32      aFilePassword = 0x00000000;

/*-----------------------------------------------------------------------------------------
------
------    application specific functions
------
-----------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------
------
------    generic functions
------
-----------------------------------------------------------------------------------------*/
/*ECATCHANGE_START(V5.12) FOE1*/
UINT16 FoE_Read(UINT16 MBXMEM * pName, UINT16 nameSize, UINT32 password, UINT16 maxBlockSize, UINT16 *pData)
{
#ifdef FW_PARSE_SREC
    UINT32 sizeError = 0;

 	UINT8 *pReadData;

	/* initialize file size */
	u32FileSize = FW_UP_APPLI_SIZE;
    
	/* file name size check */
    if ((nameSize + 1) > MAX_FILE_NAME_SIZE)
    {
        return ECAT_FOE_ERRCODE_DISKFULL;
    }
	/* file name header matching  check */
    if ((pName[0] == aFileNameHeader[0])
        && (pName[1] == aFileNameHeader[1])
        && (pName[2] == aFileNameHeader[2])
        && (pName[3] == aFileNameHeader[3])
        && (pName[4] == aFileNameHeader[4])
        && (pName[5] == aFileNameHeader[5])
        )
    {
		/* password check */
		if(aFilePassword != password)
		{
		    printf("%s %d\n",__FUNCTION__,__LINE__);
			return ECAT_FOE_ERRCODE_NORIGHTS;
		}
	}
	else
	{
	    printf("%s %d\n",__FUNCTION__,__LINE__);
        return ECAT_FOE_ERRCODE_DISKFULL;
	}
    sizeError = maxBlockSize;

    if (u32FileSize < sizeError)
    {
        sizeError = u32FileSize;
    }

	/* initilize read data address */
#if (BANK == 0)
	ReadDataAddr = (FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET);
#elif (BANK == 1)
    ReadDataAddr = (FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET);
#endif
	pReadData = ( UINT8 *)ReadDataAddr;
	
    /*copy the first foe data block*/
    MEMCPY(pData, pReadData, sizeError);

    return (UINT16)sizeError;
#endif
}

UINT16 FoE_ReadData(UINT32 offset, UINT16 maxBlockSize, UINT16 *pData)
{
#ifdef FW_PARSE_SREC
    UINT32 sizeError = 0;
	UINT8 *pReadData;

    if (u32FileSize < offset)
    {
        return 0;
    }

    /*get file length to send*/
    sizeError = u32FileSize - offset;


    if (sizeError > maxBlockSize)
    {
        /*transmit max block size if the file data to be send is greater than the max data block*/
        sizeError = maxBlockSize;
    }
	/* calculate read data address */
	pReadData = (UINT8 *)(ReadDataAddr + offset);
    /*copy the foe data block 2 .. n*/
	MEMCPY(pData, pReadData, sizeError);
    return (UINT16)sizeError;
#endif
}


UINT16 FoE_WriteData(UINT16 MBXMEM * pData, UINT16 Size, BOOL bDataFollowing)
{

    if (bBootMode)
    {
        return BL_Data(pData, Size);
    }
    else
        if ((nFileWriteOffset + Size) > MAX_FILE_SIZE)
        {
            return ECAT_FOE_ERRCODE_DISKFULL;
        }

    if (Size)
    {

        MBXMEMCPY(&aFileData[nFileWriteOffset], pData, Size);

    }

    if (bDataFollowing)
    {
        /* FoE-Data services will follow */
        nFileWriteOffset += Size;
        
    }
    else
    {
        /* last part of the file is written */
        u32FileSize = nFileWriteOffset + Size;
        nFileWriteOffset = 0;
    }

    return 0;

}
static UINT16 buf[20];
UINT16 FoE_Write(UINT16 MBXMEM * pName, UINT16 nameSize, UINT32 password)
{
    memcpy(buf, pName, nameSize);
    if ((nameSize >= SIZEOF(aFileNameHeader))
        && (pName[0] == aFileNameHeader[0])
        && (pName[1] == aFileNameHeader[1])
        && (pName[2] == aFileNameHeader[2])
        && (pName[3] == aFileNameHeader[3])
#if 0//twincat3 uncheck fw name
        && (pName[4] == aFileNameHeader[4])
        && (pName[5] == aFileNameHeader[5])
        && (pName[6] == aFileNameHeader[6])
        && (pName[7] == aFileNameHeader[7])
#endif
        )
    {
        if (bBootMode)
        {
			if(aFilePassword != password)
			{
			    LOG_DEBUG("%s %d\n",__FUNCTION__,__LINE__);
				return ECAT_FOE_ERRCODE_NORIGHTS;
			}
            BL_StartDownload(password);
            return 0;
        }
        else
        {
            LOG_DEBUG("%s %d\n",__FUNCTION__,__LINE__);
            return ECAT_FOE_ERRCODE_BOOTSTRAPONLY;
        }
    }
    else
        if (bBootMode)
        {
            LOG_DEBUG("%s %d\n",__FUNCTION__,__LINE__);
            return ECAT_FOE_ERRCODE_NOTINBOOTSTRAP;
        }
        else
    if (nameSize < MAX_FILE_NAME_SIZE)
    {
        /* for test every file name can be written */
        MBXSTRCPY(aFileName, pName, nameSize);
        MBXSTRCPY(aFileName + nameSize, "\0", 1); //string termination


        nFileWriteOffset = 0;
        u32FileSize = 0;
        return 0;
    }
    else
    {
        LOG_DEBUG("%s %d\n",__FUNCTION__,__LINE__);
        return ECAT_FOE_ERRCODE_DISKFULL;
    }

}

/*ECATCHANGE_END(V5.12) FOE1*/

/** @} */

