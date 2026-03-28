/*
* This source file is part of the EtherCAT Slave Stack Code licensed by Beckhoff Automation GmbH & Co KG, 33415 Verl, Germany.
* The corresponding license agreement applies. This hint shall not be removed.
* https://www.beckhoff.com/media/downloads/slave-stack-code/ethercat_ssc_license.pdf
*/

/**
\addtogroup ESM EtherCAT State Machine
@{
*/

/**
\file bootmode.c
\author EthercatSSC@beckhoff.com
\brief Implementation

\version 5.12

<br>Changes to version V4.20:<br>
V5.12 BOOT2: call BL_Start() from Init to Boot<br>
<br>Changes to version - :<br>
V4.20: File created
*/

/*--------------------------------------------------------------------------------------
------
------    Includes
------
--------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "ecatfoe.h"
#include "ecat_def.h"
#include "ecatslv.h"
#include "mailbox.h"
#include "ecatappl.h"
#include "foeappl.h"
#include "sampleappl.h"
#include "renesashw.h"
#include "bootmode.h"

#ifdef FW_PARSE_SREC
#define    DEBUG 					1
#define    DIRECR_ES_WR_LDR 		0
#define    _BOOTMODE_ 1
#include "bootmode.h"
#undef _BOOTMODE_

#ifdef __ICCARM__
#include "intrinsics.h"												// intrinsic functions header
#endif // __iccarm__

/* Access to Firmware updata API */
#include "hal_data.h"
#include "r_fw_up_rz_if.h"

bool BL_IsSectorErased(uint16_t offset_sector_number);
void BL_EraseSector(uint16_t offset_sector_number);
extern void handle_error(fsp_err_t err);

/*--------------------------------------------------------------------------------------
------
------    local Types and Defines
------
--------------------------------------------------------------------------------------*/
#define BL_WRITE_BUFFER_SIZE		FW_UP_PAGE_SIZE		// Byte
#define BL_DATA_STATUS_IDLE			(0)					// Idle
#define BL_DATA_STATUS_ERASE_START	(1)					// Flash Erase
#define BL_DATA_STATUS_ERASE		(2)					// Flash Erase
#define BL_DATA_STATUS_WRITE		(3)					// Data write to Flash
#define BL_DATA_STATUS_ERASE_LDRPRM	(4)					// LdrPrm Erase
#define BL_DATA_STATUS_WRITE_LDRPRM	(5)					// LdrPrm Update
#define BL_DATA_STATUS_SII_UPDATE	(6)					// SII Update

/*-----------------------------------------------------------------------------------------
------
------    local variables and constants
------
-----------------------------------------------------------------------------------------*/
static UINT8  FlashWriteBuffer[BL_WRITE_BUFFER_SIZE];
static UINT8  EraseSectorNumber;
static UINT16 EraseSectorFlag;
static UINT8  DataStatus = BL_DATA_STATUS_IDLE;
static BOOL   bReBoot;

/*-----------------------------------------------------------------------------------------
------
------    Module internal function declarations
------
-----------------------------------------------------------------------------------------*/
static void BL_SectorIsErased(uint16_t offset_sector_number);
/*-----------------------------------------------------------------------------------------
------
------    Module internal variable definitions
------
-----------------------------------------------------------------------------------------*/
BSP_DONT_REMOVE const uint32_t g_identify[4] BSP_PLACE_IN_SECTION(".identify") = {(VENDOR_ID), (PRODUCT_CODE), (REVISION_NUMBER), (SERIAL_NUMBER)};

/******************************************************************************
* Function Name: BL_Start
* Description  : Boot Loader start function
* Arguments    : State  -- Current State
* Return Value : None
******************************************************************************/
void BL_Start( UINT8 State)
{
	(void)State;
#if DEBUG
	printf("%s State=%d\n", __FUNCTION__,State);
#endif
#if 0
	char buffer[] = "BL_Start\n";
	R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t*) &buffer, 9);
#endif

} /* BL_Start() */

/******************************************************************************
* Function Name: BL_Stop
* Description  : Called in the state transition from BOOT to Init
* Arguments    : None
* Return Value : None
******************************************************************************/
void BL_Stop(void)
{
#if DEBUG
	printf("%s\n", __FUNCTION__);
#endif
}

/******************************************************************************
* Function Name: BL_StartDownload
* Description  : File download start function
* Arguments    : password -- download password
* Return Value : None
******************************************************************************/
void BL_StartDownload(UINT32 password)
{
	fw_up_return_t status;

	status = fw_up_open();					// Initialize firmware update function
	handle_error((fsp_err_t)status);

	EraseSectorNumber = 0;
	EraseSectorFlag = 0;

#if (BANK == 0)
	/* Copy the loader param of BANK0 to FlashWriteBuffer. */
	FWUPMEMCPY(&FlashWriteBuffer[0], (UINT8 *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET), FW_UP_PAGE_SIZE);
#elif (BANK == 1)
    /* Copy the loader param of BANK1 to FlashWriteBuffer. */
    FWUPMEMCPY(&FlashWriteBuffer[0], (UINT8 *)(FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET), FW_UP_PAGE_SIZE);
#endif

	DataStatus = BL_DATA_STATUS_ERASE_LDRPRM;

	FSP_PARAMETER_NOT_USED(password);
#if DEBUG
	printf("%s FlashWriteBuffer:\n", __FUNCTION__);
	for(int i = 20; i < 24; i++)
	{
		printf("%02X", FlashWriteBuffer[i]);
	}printf("\n\n");
#endif
} /* BL_StartDownload() */

/******************************************************************************
* Function Name: BL_Data
* Description  : File data receive function
* Arguments    : pData -- Data pointer
*              : Size  -- Data Length
* Return Value : FoE error code
******************************************************************************/
EEPBUFFER     Buffer;

UINT16 BL_Data(UINT16 *pData,UINT16 Size)
{
	UINT16 ErrorCode = 0;
	UINT8  LastData;
	UINT32 i;
	UINT32 ldr_addr;

	fw_up_return_t status;
    spi_flash_status_t status_erase;

#if (BANK == 0)
    volatile UINT32 *pIdentify = (UINT32 *)(FW_UP_BANK1_ADDR + FW_UP_APPLI_ID_OFFSET - FW_UP_MIRROR_OFFSET);    // Identify section address
#elif (BANK == 1)
    volatile UINT32 *pIdentify = (UINT32 *)(FW_UP_BANK0_ADDR + FW_UP_APPLI_ID_OFFSET - FW_UP_MIRROR_OFFSET);    // Identify section address
#endif

#if DEBUG
	printf("->S%d ", DataStatus);
#endif
	switch(DataStatus)
	{
	case BL_DATA_STATUS_ERASE_START:
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		if(true != status_erase.write_in_progress)														// In write progress ?
		{
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-1S-1S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
#endif

#if (BANK == 0)
			/* Erase 1sector(64KB) of BANK1 */
			R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK1_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),
							  (uint32_t)FW_UP_SECTOR_SIZE);
			printf("BL_DATA_STATUS_ERASE_START erase %x %d\n",(uint8_t *)(FW_UP_BANK1_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),FW_UP_SECTOR_SIZE);
#elif (BANK == 1)
            /* Erase 1sector(64KB) of BANK0 */
            R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK0_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),
                              (uint32_t)FW_UP_SECTOR_SIZE);
            printf("BL_DATA_STATUS_ERASE_START erase %x %d\n",(uint8_t *)(FW_UP_BANK0_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),FW_UP_SECTOR_SIZE);
#endif

			DataStatus = BL_DATA_STATUS_ERASE;

		}
		ErrorCode = FOE_MAXBUSY_ZERO;				// return "BUSY"
#if DEBUG
		printf("%s BL_DATA_STATUS_ERASE:_START FW_UP_BANK1_ADDR:EraseSectorNumber=%d\n", __FUNCTION__, EraseSectorNumber);
		printf("change=%d\n\n", DataStatus);
#endif
		break;

	case BL_DATA_STATUS_ERASE:
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		if(true != status_erase.write_in_progress)														// Is flash erasing end ?
		{
			BL_SectorIsErased(EraseSectorNumber);   // Update EraseSectorFlag.
			status = analyze_and_write_data((const uint8_t *)pData, (uint32_t)Size);					// yes. data copy to write buffer
			handle_error((fsp_err_t)status);
			DataStatus = BL_DATA_STATUS_WRITE;
		}
		else
		{
			ErrorCode = FOE_MAXBUSY_ZERO;									// no. return "BUSY"
		}
#if DEBUG
		printf("%s BL_DATA_STATUS_ERASE: Size=%d \n", __FUNCTION__,Size);
		printf("change=%d\n\n", DataStatus);
#endif
		break;

	case BL_DATA_STATUS_WRITE:
		LastData = (Size != (u16ReceiveMbxSize - MBX_HEADER_SIZE - FOE_HEADER_SIZE));
		status = analyze_and_write_data((const uint8_t *)pData, (uint32_t)Size);	// data copy to write buffer and write to flash
		
		handle_error((fsp_err_t)status);
		if(LastData == TRUE)											// last receive data ?
		{
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-4S-4S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);
#endif
#if (BANK == 0)
			/* Copy the loader param of BANK1 to FlashWriteBuffer. */
			FWUPMEMCPY(&FlashWriteBuffer[0], (UINT8 *)(FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET), FW_UP_PAGE_SIZE);
			printf("BL_DATA_STATUS_WRITE LastData=%x\n",(UINT8 *)(FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET));
#elif (BANK == 1)
            /* Copy the loader param of BANK0 to FlashWriteBuffer. */
            FWUPMEMCPY(&FlashWriteBuffer[0], (UINT8 *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET), FW_UP_PAGE_SIZE);
            printf("BL_DATA_STATUS_WRITE LastData=%x\n",(UINT8 *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET));
#endif

			DataStatus = BL_DATA_STATUS_ERASE_LDRPRM;					// yes. next -> erase ldrprm.
			ErrorCode = FOE_MAXBUSY_ZERO;								// return "BUSY"
#if DEBUG
			printf("%s BL_DATA_STATUS_WRITE:\n", __FUNCTION__);
			for(int j = 20; j < 24; j++)
			{
				printf("%02X", FlashWriteBuffer[j]);
			}printf("\n\n");

			printf("change=%d\n\n", DataStatus);
#endif
		}
		break;

	case BL_DATA_STATUS_ERASE_LDRPRM:
#if DIRECR_ES_WR_LDR
		DataStatus = BL_DATA_STATUS_WRITE_LDRPRM;
		printf("DIRECR_ES_WR_LDR\n");
		printf("change=%d\n\n", DataStatus);
		//R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
		ErrorCode = FOE_MAXBUSY_ZERO;
#else
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		if(true != status_erase.write_in_progress)							// Is flash erasing end ?
		{
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-1S-1S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
#endif
			/* Erase flash memory from 0x6000_0000 to 0x6000_1000 */
			R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)FW_UP_LDRPRM_ADDR, FW_UP_SECTOR_SIZE_4K);
			DataStatus = BL_DATA_STATUS_WRITE_LDRPRM;							// next -> update ldrprm.
			printf("BL_DATA_STATUS_ERASE_LDRPRM FW_UP_LDRPRM_ADDR=%x\n",FW_UP_LDRPRM_ADDR);
		}
		ErrorCode = FOE_MAXBUSY_ZERO;										// return "BUSY"
#if DEBUG
		printf("%s BL_DATA_STATUS_ERASE_LDRPRM: FW_UP_LDRPRM_ADDR FW_UP_SECTOR_SIZE_4K\n", __FUNCTION__);
		printf("change=%d\n\n", DataStatus);
#endif//DEBUG
#endif//DIRECR_ES_WR_LDR
		break;

	case BL_DATA_STATUS_WRITE_LDRPRM:
#if DIRECR_ES_WR_LDR
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		if(true != status_erase.write_in_progress)							// Is flash erasing end ?
		{
			R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)FW_UP_LDRPRM_ADDR, FW_UP_SECTOR_SIZE_4K);
			//DataStatus = BL_DATA_STATUS_WRITE_LDRPRM;							// next -> update ldrprm.
			R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
		}
		else
		{
			ErrorCode = FOE_MAXBUSY_ZERO;										// return "BUSY"
			break;
		}
#endif//DIRECR_ES_WR_LDR
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		if(true != status_erase.write_in_progress)							// Is flash erasing end ?
		{
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-1S-1S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
#endif
			/* Write FlashWriteBuffer[0 - 255] to loder param area (from 0x6000_0000 to 0x6000_1000)*/
			for ( i = 0; i < FW_UP_PAGE_SIZE / FW_UP_WRITE_ATONCE_SIZE ; i++)
			{
				R_XSPI_QSPI_Write(&g_qspi0_ctrl, &FlashWriteBuffer[0 + (i * FW_UP_WRITE_ATONCE_SIZE)], 
								  (uint8_t *)(FW_UP_LDRPRM_ADDR + (i * FW_UP_WRITE_ATONCE_SIZE) - FW_UP_MIRROR_OFFSET),
								  (uint32_t)FW_UP_WRITE_ATONCE_SIZE);
				do
				{
					(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
				} while (true == status_erase.write_in_progress);
			}
#if (FW_UP_FLASH_TYPE == 1)
			/* For following sequnces, set protocol 1S-4S-4S */
			R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);
#endif
			ldr_addr = *(UINT32 *)(&FlashWriteBuffer[FW_UP_LDRPRM_LDR_ADDR - FW_UP_LDRPRM_ADDR]);
			printf("BL_DATA_STATUS_WRITE_LDRPRM ldr_addr=%x\n",ldr_addr);
#if (BANK == 0)
			if((ldr_addr & FW_UP_BANK0_ADDR) == FW_UP_BANK0_ADDR)			// recover BANK0 LDRPRM ?
#elif (BANK == 1)
            if((ldr_addr & FW_UP_BANK1_ADDR) == FW_UP_BANK1_ADDR)           // recover BANK1 LDRPRM ?
#endif
			{
				DataStatus = BL_DATA_STATUS_ERASE_START;					// yes. next -> erase BANK
			}
			else
			{
				DataStatus = BL_DATA_STATUS_SII_UPDATE;						// No. write BANK LDRPRM next -> update SII.
			}
			ErrorCode = FOE_MAXBUSY_ZERO;									// return "BUSY"
		}
		else
		{
			ErrorCode = FOE_MAXBUSY_ZERO;									// no. return "BUSY"
		}
#if DEBUG
		printf("%s BL_DATA_STATUS_WRITE_LDRPRM: ldr_addr=%x\n", __FUNCTION__,ldr_addr);
		for(int k = 20; k < 24; k++)
		{
			printf("%02X", FlashWriteBuffer[k]);
		}printf("\n\n");
		printf("change=%d\n\n", DataStatus);
#endif
		break;

	case BL_DATA_STATUS_SII_UPDATE:
#if (FW_UP_FLASH_TYPE == 1)
		/* For following sequnces, set protocol 1S-4S-4S */
		R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);
#endif
		//--------------------------------------------------
		// SII update, update firmware Revision Number
		//--------------------------------------------------
		for ( i = 0; i < 4 ; i++)											// get new firmware identify 
		{
			Buffer.dword[i] = *pIdentify++;
		}
		ESC_EepromAccess(SII_EEP_IDENTIFY_OFFSET + SII_EEP_REVESIONNO, 2, &Buffer.word[SII_EEP_REVESIONNO], ESC_WR);
		printf("BL_DATA_STATUS_SII_UPDATE Buffer.word[SII_EEP_REVESIONNO]=%x\n",Buffer.word[SII_EEP_REVESIONNO]);
		fw_up_close();
		BL_SetRebootFlag(TRUE);												// yes. reboot is available.
#if DEBUG
		printf("%s BL_DATA_STATUS_SII_UPDATE: revision=%x\n", __FUNCTION__,Buffer.word[SII_EEP_REVESIONNO]);
		printf("change=%d\n\n", DataStatus);
#endif
		break;
	case BL_DATA_STATUS_IDLE:
	default:
		break;
	}

	return(ErrorCode);
} /* BL_Data() */

/******************************************************************************
* Function Name: BL_SetRebootFlag
* Description  : Reboot flag set function
* Arguments    : Flag -- TRUE/FALSE
* Return Value : None
******************************************************************************/
void BL_SetRebootFlag(BOOL Flag)
{
	bReBoot = Flag;
}

/******************************************************************************
* Function Name: BL_CheckRebootFlag
* Description  : Check reboot flag function
* Arguments    : None
* Return Value : Flag
******************************************************************************/
BOOL BL_CheckRebootFlag(void)
{
	return(bReBoot);
}


/******************************************************************************
* Function Name: BL_Reboot
* Description  : Reboot boot loader function
* Arguments    : None
* Return Value : None
******************************************************************************/
void BL_Reboot(void)
{
#if (FW_UP_DUAL_BANK_MODE)	// Daul mode
	uint32_t reset_vector_value;
    fw_up_return_t flash_status;
    bool reset_vector_ok = false;

	/* get reset vector of the BANK1 */
	reset_vector_value = *(uint32_t*)(Bank[FW_UP_BANK1].high_addr - (sizeof(uint32_t*) - 1u));
	/* The reset vector is checked to see if the address is within the range of BANK0 */
	if(reset_vector_value != FW_UP_BLANK_VALUE)
	{
		reset_vector_ok = fw_up_check_addr_value(reset_vector_value, FW_UP_BANK0);
	}

    if (reset_vector_ok)
    {
        flash_status = bank_toggle();
        if (FW_UP_SUCCESS == flash_status)
        {
            /* soft reset */
            fw_up_soft_reset();
        }
        else
        {
            // Do Nothing!
        }
    }
    else
    {
        // Do Nothing!
    }

#else  // if (FW_UP_DUAL_BANK_MODE)

	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_RESET);
	R_BSP_SystemReset();	// System Software Reset
	while(1)
	{
		/* Do nothing */
	};
#endif  // if (FW_UP_DUAL_BANK_MODE)

} /* BL_Reboot() */

/******************************************************************************
* Function Name: BL_IsSectorErased
* Description  : Check if sector is erased 
* Arguments    : Sector number
* Return Value : None
******************************************************************************/
bool BL_IsSectorErased(uint16_t offset_sector_number)
{
	if((1U << offset_sector_number) & EraseSectorFlag)
	{
		return(true);
	}
	else
	{
		return(false);
	}
} /* BL_IsSectorErased */

/******************************************************************************
* Function Name: BL_EraseSector
* Description  : Erase the sector
* Arguments    : Sector number
* Return Value : None
******************************************************************************/
void BL_EraseSector(uint16_t offset_sector_number)
{
	volatile uint16_t dummy16;
    spi_flash_status_t status_erase;

	EraseSectorNumber = (uint8_t)offset_sector_number;
#if (FW_UP_FLASH_TYPE == 1)
    /* For following sequnces, set protocol 1S-1S-1S */
    R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
#endif
#if (BANK == 0)
    /* Erase 1sector(64KB) of BANK1 */
    R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK1_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),
					  (uint32_t)FW_UP_SECTOR_SIZE);
#elif (BANK == 1)
    /* Erase 1sector(64KB) of BANK0 */
    R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK0_ADDR + (EraseSectorNumber * FW_UP_SECTOR_SIZE)),
                      (uint32_t)FW_UP_SECTOR_SIZE);
#endif

	do
	{
		(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
		HW_EscReadWord(dummy16, ESC_EEPROM_CONFIG_OFFSET);			// Countermeasure against PD watchdog timeout
		R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);

	} while (true == status_erase.write_in_progress);

	BL_SectorIsErased(EraseSectorNumber);

	FSP_PARAMETER_NOT_USED(dummy16);
} /* BL_EraseSector */

/******************************************************************************
* Function Name: BL_SectorIsErased
* Description  : Set erased sector flag
* Arguments    : Sector number
* Return Value : None
******************************************************************************/
static void BL_SectorIsErased(uint16_t offset_sector_number)
{
	EraseSectorFlag |= (uint16_t)(1U << offset_sector_number);
} /* BL_SectorIsErased */

#else


#include "flash_config.h"
#include "circular_queue.h"
#include "crc32_table.h"
#include "bsp_r52_global_counter.h"
#include "sbl_params.h"
#include "ecat_foe_data.h"
#include "log.h"

#define CURRENT_LOG_LEVEL   LOG_LEVEL_DEBUG

#define    DEBUG                    1



static ota_handle_t ota_handle = {0};
static volatile uint64_t beginTime = 0;
static volatile uint64_t endTime = 0;
CRC_Context ctx;
static BOOL   bReBoot;

#if (BANK == 0)
BSP_DONT_REMOVE const uint8_t g_header[HEADER_PARAMS_LENS] BSP_PLACE_IN_SECTION(".header") = {"APP1BANK0" };
#elif (BANK == 1)
BSP_DONT_REMOVE const uint8_t g_header[HEADER_PARAMS_LENS] BSP_PLACE_IN_SECTION(".header") = {"APP1BANK1" };
#endif
BSP_DONT_REMOVE const uint32_t g_identify[4] BSP_PLACE_IN_SECTION(".identify") = {(VENDOR_ID), (PRODUCT_CODE), (REVISION_NUMBER), (SERIAL_NUMBER)};


static void norFlashPageProgram(uint8_t* addr, uint8_t* data, uint16_t len);
/*
 * page program 256Byte
 *
 */
static void norFlashPageProgram(uint8_t* addr, uint8_t* data, uint16_t len)
{
    (void)len;

    spi_flash_status_t status_erase;
    for ( uint8_t i = 0; i < FW_UP_PAGE_SIZE / FW_UP_WRITE_ATONCE_SIZE ; i++)
    {
        R_XSPI_QSPI_Write(&g_qspi0_ctrl, &data[0 + (i * FW_UP_WRITE_ATONCE_SIZE)],
                          (uint8_t *)(addr + (i * FW_UP_WRITE_ATONCE_SIZE) - FW_UP_MIRROR_OFFSET),
                          (uint32_t)FW_UP_WRITE_ATONCE_SIZE);
        do
        {
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
        } while (true == status_erase.write_in_progress);
    }
}

void test_byte_array(void);
void test_byte_array(void) {
//    printf("Test 5 - Byte array {0x00, 0x01, 0x02, 0x03, 0x04}:\n");
//    uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04};
//    //uint32_t crc = calculate_crc32(data, sizeof(data), 0xFFFFFFFF, 0xFFFFFFFF);
//    printf("  CRC32 = 0x%08X\n", crc);
//    printf("  Expected: 0xB63CFBCD\n");
//    printf("  Result: %s\n\n", (crc == 0xB63CFBCD) ? "PASS" : "FAIL");
}

void BL_Start( UINT8 State)
{
    (void)State;

    CRC_Init(&ctx, 0xFFFFFFFF, 0xEDB88320);  // 标准CRC32参数

#if 0//test crc32
    LOG_DEBUG("=== CRC32 Test ===\n\n");

    // 测试数据
    const char* test_str = "123456789";
    const uint8_t test_bytes[] = {0x00, 0x01, 0x02, 0x03, 0x04};
    LOG_DEBUG("1. Using CRC_Context:\n");
    uint32_t crc1 = CRC_Calculate(&ctx, test_str, strlen(test_str));
    LOG_DEBUG("   CRC32 of \"123456789\": 0x%08X (Expected: 0xCBF43926)\n", crc1);
#endif

    memset(&ota_handle, 0 , sizeof(ota_handle_t));
    Queue_Init((Circular_queue_t*)&Circular_queue);

#if DEBUG
    LOG_INFO("%s State=%d\n", __FUNCTION__,State);
#endif
#if 0
    char buffer[] = "BL_Start\n";
    R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t*) &buffer, 9);
#endif
}

void BL_Stop(void)
{
#if DEBUG
    LOG_INFO("%s\n", __FUNCTION__);
#endif
}



void BL_StartDownload(UINT32 password)
{
    (void)password;

    beginTime =  getGlobalCounter();

    checkAndUpdataBootBankParams();

    // may be only erase 64KB one time, try 128KB failed
    for(uint8_t i = 0; i < FW_UP_TOTAL_SIZE/FW_UP_SECTOR_SIZE; i++)
    {

#if (BANK == 0)
        /* Erase 1sector of BANK1 */
        R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK1_ADDR + (i * FW_UP_SECTOR_SIZE)),(uint32_t)FW_UP_SECTOR_SIZE);
#elif (BANK == 1)
        /* Erase 1sector of BANK0 */
        R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK0_ADDR + (i * FW_UP_SECTOR_SIZE)),(uint32_t)FW_UP_SECTOR_SIZE);
#endif

        spi_flash_status_t status_erase;
        volatile uint16_t dummy16;
        do
        {
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
            HW_EscReadWord(dummy16, ESC_EEPROM_CONFIG_OFFSET);          // Countermeasure against PD watchdog timeout
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            (void)dummy16;

        } while (true == status_erase.write_in_progress);
    }
}

UINT16 BL_Data(UINT16 *pData,UINT16 Size)
{
    UINT16 ErrorCode = 0;


    if(ota_handle.recv_offset == 0)
    {
        app_header_t *current_header = (app_header_t *)pData;
        memcpy(&ota_handle.current_header, current_header, sizeof(app_header_t));

        //todo
        //校验当前运行bank与接收到的bank是否一致，一致则失败，不同则继续
    }

    Queue_Wirte(&Circular_queue, (uint8_t*) ((uint8_t*)pData), Size);
    if(Queue_HadUse(&Circular_queue) >= FW_UP_PAGE_SIZE)
    {
        Queue_Read(&Circular_queue, ota_handle.Read_Buffer, FW_UP_PAGE_SIZE);
#if (BANK == 0)
        norFlashPageProgram((uint8_t *)(FW_UP_BANK1_ADDR + ota_handle.write_offset), ota_handle.Read_Buffer, FW_UP_PAGE_SIZE);
#elif (BANK == 1)
        norFlashPageProgram((uint8_t *)(FW_UP_BANK0_ADDR + ota_handle.write_offset), ota_handle.Read_Buffer, FW_UP_PAGE_SIZE);
#endif
        ota_handle.write_offset += FW_UP_PAGE_SIZE;
    }

    // 1 to n-1 times
    if((ota_handle.recv_offset > 0) && (ota_handle.current_header.header_len - ota_handle.recv_offset > FW_UP_PACKAGE_SIZE))
    {
        LOG_INFO("P:%ld%% ", ota_handle.write_offset*100/ota_handle.current_header.header_len);
    }
    // n last time and lastDataLen < 256
    else if((ota_handle.recv_offset > 0) && (ota_handle.current_header.header_len - ota_handle.recv_offset <= FW_UP_PACKAGE_SIZE))
    {
        uint16_t lastDataLen = Queue_HadUse(&Circular_queue);

        Queue_Read(&Circular_queue, ota_handle.Read_Buffer, lastDataLen);
#if (BANK == 0)
        norFlashPageProgram((uint8_t *)(FW_UP_BANK1_ADDR + ota_handle.write_offset), ota_handle.Read_Buffer, lastDataLen);
#elif (BANK == 1)
        norFlashPageProgram(FW_UP_BANK0_ADDR + ota_handle.write_offset, ota_handle.Read_Buffer, FW_UP_PAGE_SIZE);
#endif
        ota_handle.write_offset += lastDataLen;

        LOG_INFO("Last P:%ld%%\n", ota_handle.write_offset*100/ota_handle.current_header.header_len);
        LOG_INFO("write flash finished!!!\n");
    }

    // after write flash, ready to do:
    // 1. read flash check crc data
    // 2. SII update, update firmware Revision Number
    // 3. write boot bank params
    if(ota_handle.write_offset >= ota_handle.current_header.header_len)
    {
        // 1. read flash check crc data
#if (BANK == 0)
        volatile uint32_t *pCheckCrc = (UINT32 *)(FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET);    // Identify section address
#elif (BANK == 1)
        volatile uint32_t *pCheckCrc = (UINT32 *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET);    // Identify section address
#endif

        uint32_t crcCalRet = CRC_Calculate(&ctx, (char*)pCheckCrc, (int)(ota_handle.current_header.header_len - 4));
        //uint32_t crcCalRet = calculate_crc32((uint8_t*)pCheckCrc, ota_handle.current_header.header_len - 4, init_value, xor_out);
        uint32_t crcFlashData = *((UINT32 *)(FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET + ota_handle.current_header.header_len - 4));

        if(crcCalRet != crcFlashData)
        {
            LOG_DEBUG("crcCalRet=%lX, crcFlashData=%lX\n", crcCalRet, crcFlashData);
            return FOE_ERROR;
        }
        else
        {
            LOG_INFO("Flash crc check succeed!!!\n");
        }

        //--------------------------------------------------
        // 2. SII update, update firmware Revision Number
        //--------------------------------------------------
        EEPBUFFER     Buffer;
        for ( uint8_t i = 0; i < 4 ; i++)                                           // get new firmware identify
        {
            Buffer.dword[i] = ota_handle.current_header.dword[i];
        }
        ESC_EepromAccess(SII_EEP_IDENTIFY_OFFSET + SII_EEP_REVESIONNO, 2, &Buffer.word[SII_EEP_REVESIONNO], ESC_WR);

        LOG_INFO("BL_DATA_STATUS_SII_UPDATE Buffer.word[SII_EEP_REVESIONNO]=%x\n",Buffer.word[SII_EEP_REVESIONNO]);

        BL_SetRebootFlag(TRUE);                                             // yes. reboot is available.


        // 3. write boot bank params
        R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK0_ADDR - FW_UP_BOOT_PARAMS_SIZE),(uint32_t)FW_UP_BOOT_PARAMS_SIZE);

        spi_flash_status_t status_erase;
        do{
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
        } while (true == status_erase.write_in_progress);

        uint8_t bootParams[FW_UP_WRITE_ATONCE_SIZE] = {0};
        uint16_t idx = 0;
        memcpy(bootParams + idx, ota_handle.current_header.header_app, HEADER_PARAMS_APP);
        idx += HEADER_PARAMS_APP;

        memcpy(bootParams + idx, ota_handle.current_header.byte, sizeof(g_identify));
        idx += sizeof(g_identify);

        uint32_t crcBootParams = CRC_Calculate(&ctx, (char*)bootParams, idx);
        memcpy(bootParams + idx, (uint8_t*)&crcBootParams, sizeof(crcBootParams));
        idx += sizeof(crcBootParams);

        LOG_DEBUG("write boot Params:");
        for(uint8_t i = 0; i < idx; i++)
        {
            LOG_DEBUG("%02X", bootParams[i]);
        }LOG_DEBUG("\n");

        R_XSPI_QSPI_Write(&g_qspi0_ctrl, bootParams,
                (uint8_t *)(FW_UP_BANK0_ADDR - FW_UP_BOOT_PARAMS_SIZE),
                                          (uint32_t)FW_UP_WRITE_ATONCE_SIZE);
        do{
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
        } while (true == status_erase.write_in_progress);

        memset(bootParams, 0, sizeof(bootParams));
        memcpy(bootParams, (uint8_t *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET - FW_UP_BOOT_PARAMS_SIZE), sizeof(bootParams));
        LOG_DEBUG("read  boot Params:");
        for(uint8_t i = 0; i < idx; i++)
        {
            LOG_DEBUG("%02X", bootParams[i]);
        }LOG_DEBUG("\n");

        uint32_t readCrc = *(uint32_t*)(bootParams + idx - 4);
        crcBootParams = CRC_Calculate(&ctx, (char*)bootParams, idx - 4);
        LOG_DEBUG("read crc:%lX\n cal crc=%lX\n", readCrc, crcBootParams);

        //todo
        //如果crc不一致，则把boot bank params恢复到当前bank数值，升级失败回滚到当前bank


        // end
        endTime =  getGlobalCounter();
        LOG_INFO("ECAT FOE Total Time:%ldms!!!\n\n", (uint32_t)(endTime - beginTime)/BSP_GLOBAL_SYSTEM_COUNTER_CLOCK_HZ*1000);

        return 0;
    }



    ota_handle.recv_offset += Size;
    //ota_handle.current_crc = calculate_crc32((uint8_t*)pData, Size, init_value, xor_out);

    return(ErrorCode);
}

void BL_SetRebootFlag(BOOL Flag)
{
    bReBoot = Flag;
}

BOOL BL_CheckRebootFlag(void)
{
    return(bReBoot);
}


void BL_Reboot(void)
{
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_RESET);
    R_BSP_SystemReset();    // System Software Reset
    while(1)
    {
        /* Do nothing */
    };
}

#endif
