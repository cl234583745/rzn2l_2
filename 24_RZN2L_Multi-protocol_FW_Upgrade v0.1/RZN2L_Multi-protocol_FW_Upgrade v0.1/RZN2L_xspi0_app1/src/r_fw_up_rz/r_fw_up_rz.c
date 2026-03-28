/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * File Name    : r_fw_up_rz.c
 * Version      : 1.00
 * Description  : Firmware update 
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 15.04.2023 1.00     First Release
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 Includes <System Includes> , "Project Includes"
 ***********************************************************************************************************************/
#ifdef FW_PARSE_SREC
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

/* Access to peripherals and board defines. */
#include "hal_data.h"

/* Defines for Firmware update support */
#include "r_fw_up_rz_if.h"
#include "r_fw_up_buf.h"
#include "r_fw_up_rz_private.h"

/*******************************************************************************
Imported global variables and functions (from other files)
*******************************************************************************/
extern bool BL_IsSectorErased(uint16_t offset_sector_number);
extern void BL_EraseSector(uint16_t offset_sector_number);
extern void handle_error(fsp_err_t err);
/***********************************************************************************************************************
 Global variables and functions
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 Private variables and functions
 ***********************************************************************************************************************/
static bool is_opened = false;


static fw_up_return_t write_firmware(fw_up_fl_data_t *p_fl_data);
static fw_up_return_t fw_up_put_data(fw_up_fl_data_t *p_fl_data);
static fw_up_return_t fw_up_get_data(fw_up_fl_data_t *p_fl_data);
static fw_up_return_t fw_up_check_erase_sector(fw_up_fl_data_t *p_fl_data);

/***********************************************************************************************************************
 * Function Name: fw_up_open
 * Description  : Initialize firmware update function.
 * Arguments    : none
 * Return Value : FW_UP_SUCCESS -
 *                   Processing completed successfully.
 *                FW_UP_ERR_OPENED -
 *                   fw_up_open has already been run.
 ***********************************************************************************************************************/
fw_up_return_t fw_up_open(void)
{
    fw_up_return_t ret = FW_UP_SUCCESS;

    /* Check that the fw_up_open() has been executed. */
    if (true == is_opened)
    {
        return FW_UP_ERR_OPENED;
    }

    /* Initialize variable for store address of allocated memory */
    fw_up_memory_init();

    /* Initialize ring buffer */
    fw_up_buf_init();

	/* Enable QPI */
	//qspi_set_Quad_Mode_enable();

    /* Set initialize complete flag */
    is_opened = true;

    return ret;
} /* End of function fw_up_open() */

/***********************************************************************************************************************
 * Function Name: fw_up_close
 * Description  : Initialize firmware update function.
 * Arguments    : none
 * Return Value : FW_UP_SUCCESS -
 *                   Processing completed successfully.
 *                FW_UP_ERR_NOT_OPEN -
 *                   fw_up_open has not been run.
 ***********************************************************************************************************************/
fw_up_return_t fw_up_close(void)
{
    fw_up_return_t ret = FW_UP_SUCCESS;

    /* Check that the fw_up_open() has been executed. */
    if (false == is_opened)
    {
        return FW_UP_ERR_NOT_OPEN;
    }

    /* Initialize ring buffer */
    fw_up_buf_init();

	/* Disable QPI */
	//qspi_set_Quad_Mode_disable();

    /* Clear initialize complete flag */
    is_opened = false;

    return ret;
} /* End of function fw_up_close() */

/***********************************************************************************************************************
 * Function Name: analyze_and_write_data
 * Description  : Analyze mot s format and program data.
 * Arguments    : *p_recv_data -
 *                   mot file data.
 *                data_size
 *                   mot file data size.m
 * Return Value : FW_UP_SUCCESS -
 *                   Processing completed successfully.
 *                FW_UP_ERR_WRITE -
 *                   Flash API(erase) error.
 ***********************************************************************************************************************/
fw_up_return_t analyze_and_write_data(const uint8_t *p_recv_data, uint32_t data_size)
{
    fw_up_return_t  ret1;
    fw_up_return_t  ret2;
    fw_up_return_t  ret3;
    fw_up_fl_data_t fl_data;
    fw_up_fl_data_t bin_data;
    int32_t status;

    /* Put received data in buffer */
    /* Casting is valid because the pointer value is 32 bits and the reference destination of the buffer is never changed */
    fl_data.src_addr = (uint32_t)p_recv_data;
    fl_data.len = data_size;
    fl_data.count = 0;

    do
    {
        /* Put received data in buffer */
        ret1 = fw_up_put_data(&fl_data);
        if (FW_UP_ERR_INVALID_RECORD == ret1)
        {
            return ret1;
        }
        ret3 = FW_UP_SUCCESS;
        do
        {
            /* Get Pointer to firmware data and data size, write address. */
            ret2 = fw_up_get_data(&bin_data);
            if (FW_UP_SUCCESS == ret2)
            {
                /* Check write address  */
                if (fw_up_check_addr_value(bin_data.dst_addr) == true)
                {
					/* Check if the sector to be written is erased */
					ret3 = fw_up_check_erase_sector(&bin_data);

					if(FW_UP_SUCCESS == ret3)
					{
						/* Flash mirror space address converion */
						bin_data.dst_addr -= FW_UP_MIRROR_OFFSET;
						
						/* Write firmware to the bank */
						ret3 = write_firmware(&bin_data);
						if (FW_UP_SUCCESS == ret3)
						{
#if (FW_UP_FLASH_TYPE == 1)
							/* For following sequnces, set protocol 1S-4S-4S */
							R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);
#endif
							/* Casting is valid because address values are converted to pointers */
							status = FWUPMEMCMP((int8_t*)(bin_data.dst_addr), (int8_t*)bin_data.src_addr, bin_data.len);

							if (0 != status)
							{
								ret3 = FW_UP_ERR_WRITE;
							}
						}
						else
						{
							ret3 = FW_UP_ERR_WRITE;
						}
					}
                }
            }
        }
        while ((FW_UP_SUCCESS == ret3) && (FW_UP_SUCCESS == ret2));
    }
    while (FW_UP_SUCCESS != ret1);

    return ret3;
} /* End of function analyze_and_write_data() */

/***********************************************************************************************************************
 * Function Name: fw_up_addr_check_addr_value
 * Description  : The address is checked valid.
 * Arguments    : none
 * Return Value : true -
 *                   Valid.
 *                false -
 *                   Invalid.
 ***********************************************************************************************************************/
bool fw_up_check_addr_value(uint32_t addr_value)
{
    bool ret = false;

	if(addr_value != FW_UP_BLANK_VALUE)
	{
#if (BANK == 0)
		if ((addr_value >= FW_UP_BANK1_ADDR && (FW_UP_BANK1_ADDR + FW_UP_APPLI_SIZE) > addr_value))
		{
			ret = true;
		}
#elif (BANK == 1)
        if ((addr_value >= FW_UP_BANK0_ADDR && (FW_UP_BANK0_ADDR + FW_UP_APPLI_SIZE) > addr_value))
        {
            ret = true;
        }
#endif
	}

    return ret;

} /* End of function fw_up_check_addr_value */

/***********************************************************************************************************************
 * Function Name: write_firmware
 * Description  : Write firmware data.
 * Arguments    : p_fl_data -
 *                   Pointer to binary data of initial address, write address, data size.
 * Return Value : FW_UP_SUCCESS -
 *                   Processing completed successfully.
 *                FW_UP_ERR_NOT_OPEN -
 *                   R_FW_UP_Open has not been run.
 *                FW_UP_ERR_NULL_PTR -
 *                   The argument p_fl_data is null pointer.
 *                FW_UP_ERR_INTERNAL -
 *                   Flash API error.
 ***********************************************************************************************************************/
static fw_up_return_t write_firmware(fw_up_fl_data_t *p_fl_data)
{
    fw_up_return_t ret = FW_UP_SUCCESS;
	fsp_err_t err = FSP_SUCCESS;

	uint32_t i, j, write_size, remain_size;
    spi_flash_status_t status_write;

    /* Check that the fw_up_open has been executed. */
    if (false == is_opened)
    {
        return FW_UP_ERR_NOT_OPEN;
    }

    /* Make sure that the argument is correct. */
    /* To cast in order to compare the address. There is no problem because the information is not lost even if the
     *  cast. */
    if (NULL == p_fl_data)
    {
        return FW_UP_ERR_NULL_PTR;
    }
#if (FW_UP_FLASH_TYPE == 1)
	/* For following sequnces, set protocol 1S-1S-1S */
	R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);
#endif
    /* Write firmware data */
	j = (p_fl_data->len / (FW_UP_WRITE_ATONCE_SIZE + 1) + 1);
	remain_size = p_fl_data->len;
	for ( i = 0; i < j ; i++)
	{
		if(remain_size >= FW_UP_WRITE_ATONCE_SIZE)
		{
			write_size = FW_UP_WRITE_ATONCE_SIZE;
			remain_size -= FW_UP_WRITE_ATONCE_SIZE;
		}
		else
		{
			write_size = remain_size;
		}
		err = R_XSPI_QSPI_Write(&g_qspi0_ctrl, 
								(uint8_t *)(p_fl_data->src_addr + (i * FW_UP_WRITE_ATONCE_SIZE)), 
								(uint8_t *)(p_fl_data->dst_addr + (i * FW_UP_WRITE_ATONCE_SIZE)), 
								write_size);
		if (FSP_SUCCESS != err)
		{
			break;
		}
		do
		{
			(void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_write);
		} while (true == status_write.write_in_progress);
	}

    if (FSP_SUCCESS != err)
    {
        ret = FW_UP_ERR_INTERNAL;
    }

    return ret;

} /* End of function write_firmware() */


/***********************************************************************************************************************
 * Function Name: fw_up_put_data
 * Description  : Put in firmware data to ring buffer
 * Arguments    : *p_fl_data -
 *                    Pointer to initial address and data size of firmware data.
 * Return Value : FW_UP_SUCCESS -
 *                    Processing completed successfully.
 *                FW_UP_ERR_NOT_OPEN -
 *                    fw_up_open has not been run.
 *                FW_UP_ERR_NULL_PTR -
 *                    The argument p_fl_data is null pointer.
 *                FW_UP_ERR_RING_BUF_FULL -
 *                    Ring buffer is not empty.
 *                FW_UP_ERR_INVALID_RECORD -
 *                    Motorola S record format data is invalid.
 *                FW_UP_ERR_INTERNAL -
 *                    Memory allocation is failed.
 ***********************************************************************************************************************/
static fw_up_return_t fw_up_put_data(fw_up_fl_data_t *p_fl_data)
{
    fw_up_return_t ret = FW_UP_SUCCESS;
    uint16_t cnt;
    uint8_t *pdata_tmp;

    /* Check that the fw_up_open() has been executed. */
    if (false == is_opened)
    {
        return FW_UP_ERR_NOT_OPEN;
    }

    /* Make sure that the argument is correct. */
    /* To cast in order to compare the address. There is no problem because the information is not lost even if the
     *  cast. */
    if (NULL == p_fl_data)
    {
        return FW_UP_ERR_NULL_PTR;
    }

    /* Set initial address of firmware data to be put in ring buffer */
    pdata_tmp = ((uint8_t *)p_fl_data->src_addr) + p_fl_data->count;

    for (cnt = p_fl_data->count; cnt < p_fl_data->len; cnt++)
    {
        /* Put firmware data in ring buffer */
        ret = fw_up_put_mot_s(*pdata_tmp);

        if (FW_UP_SUCCESS != ret)
        {
            /* Set values of data size to be put in ring buffer */
            p_fl_data->count = cnt;

            return ret;
        }

        pdata_tmp++;
    }

    return ret;

} /* End of function fw_up_put_data() */


/***********************************************************************************************************************
 * Function Name: fw_up_get_data
 * Description  : Stores firmware update data to ring buffer
 * Arguments    : *p_fl_data -
 *                   Pointer to binary data of initial address.
 * Return Value : FW_UP_SUCCESS -
 *                   Processing completed successfully.
 *                FW_UP_ERR_NOT_OPEN -
 *                   fw_up_open has not been run.
 *                FW_UP_ERR_NULL_PTR -
 *                   The argument p_fl_data is null pointer.
 *                FW_UP_ERR_RING_BUF_EMPTY -
 *                   Ring buffer is empty.
 ***********************************************************************************************************************/
static fw_up_return_t fw_up_get_data(fw_up_fl_data_t *p_fl_data)
{
    fw_up_return_t ret = FW_UP_SUCCESS;

    /* Check that the R_FW_UP_Open has been executed. */
    if (false == is_opened)
    {
        return FW_UP_ERR_NOT_OPEN;
    }

    /* Make sure that the argument is correct. */
    /* To cast in order to compare the address. There is no problem because the information is not lost even if the
     *  cast. */
    if (NULL == p_fl_data)
    {
        return FW_UP_ERR_NULL_PTR;
    }

    /* Get initial address and write address, data size of firmware data */
    ret = fw_up_get_binary(p_fl_data);

    return ret;
} /* End of function fw_up_get_data() */

/***********************************************************************************************************************
 * Function Name: fw_up_check_erase_sector
 * Description  : Erase the sector to be written if it has not been erased.
 * Arguments    : *p_fl_data -
 *                   Pointer to binary data of initial address.
 * Return Value : FW_UP_SUCCESS -
 *                   Processing completed successfully.
 ***********************************************************************************************************************/
static fw_up_return_t fw_up_check_erase_sector(fw_up_fl_data_t *p_fl_data)
{
    fw_up_return_t ret;
	uint16_t begin_sector_number;
	uint16_t end_sector_number;

	begin_sector_number = (p_fl_data->dst_addr & 0x000f0000) >> 16;
	end_sector_number   = ((p_fl_data->dst_addr + p_fl_data->len) & 0x000f0000) >> 16;

	if(BL_IsSectorErased(begin_sector_number) == false)
	{
		BL_EraseSector(begin_sector_number);
	}
	if(BL_IsSectorErased(end_sector_number) == false)
	{
		BL_EraseSector(end_sector_number);
	}
	ret = FW_UP_SUCCESS;
	return ret;
} /* End of function fw_up_check_erase_sector */

/***********************************************************************************************************************
 * Function Name: fw_up_memcmp
 * Description  : memory compare
 * Arguments    : *p_dst - 
 *                   Pointer to data of destination address.
 *                *p_src - 
 *                   Pointer to data of source address.
 *                len - 
 *                   length of data copmpare
 * Return Value : 0 - memaory data match , 1 or -1 - memory data not match
 ***********************************************************************************************************************/
int32_t fw_up_memcmp(int8_t *p_dst, int8_t *p_src, uint32_t len)
{
	int32_t	ret = 0;
	uint32_t i;
    volatile uint8_t dummy_read;

	for (i = 0; i < len; i++)
	{
		if (*p_dst < *p_src)
		{
			
			ret = -1;
			break;
		}
		else  if (*p_dst > *p_src)
		{
			ret = 1;
			break;
		}
		else
		{
			/* Do nothing! */
		}
        /* If least significant 2 bits of p_dst address are 0x02, put dummy read sequence to deactivate unintentional Continuous Read Mode */
		if (((uint32_t)p_dst & 0x03) == 0x02)
		{
            dummy_read = *(uint8_t *)(FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET);
		}
		p_src++;
		p_dst++;
	}
	(void) dummy_read;
	return ret;
} /* End of fw_up_memcmp */

/***********************************************************************************************************************
 * Function Name: fw_up_memcpy
 * Description  : memory copy
 * Arguments    : *p_dst - 
 *                   Pointer to data of destination address.
 *                *p_src - 
 *                   Pointer to data of source address.
 *                len - 
 *                   length of data copy
 * Return Value : none
 ***********************************************************************************************************************/
void fw_up_memcpy(uint8_t *p_dst, uint8_t *p_src, uint32_t len)
{
	uint32_t i;
    volatile uint8_t dummy_read;

	for (i = 0; i < len; i++)
	{
		*p_dst = *p_src;

        /* If least significant 2 bits of p_dst address are 0x02, put dummy read sequence to deactivate unintentional Continuous Read Mode */
		if (((uint32_t)p_dst & 0x03) == 0x02)
		{
            dummy_read = *(uint8_t *)(FW_UP_BANK1_ADDR - FW_UP_MIRROR_OFFSET);
		}
		p_src++;
		p_dst++;
	}
	(void) dummy_read;

} /* End of fw_up_memcpy */

/***********************************************************************************************************************
 * Function Name: qspi_set_Quad_Mode_enable
 * Description  : Enable QPI mode
 * Arguments    : non
 * Return Value : non
 ***********************************************************************************************************************/
void qspi_set_Quad_Mode_enable(void)
{
#if (FW_UP_FLASH_TYPE == 0)
    /* For following settings, temporary set protocol 1S-1S-1S */
    R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);

    /*--------------- Read status register(RDSR) (05h) ---------------*/
    spi_flash_direct_transfer_t direct_command_RDSR = {0};
    direct_command_RDSR.command        = 0x05U;
    direct_command_RDSR.command_length = 1U;
    direct_command_RDSR.data_length    = 1U;

    fsp_err_t err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command_RDSR, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    handle_error(err);
    uint32_t read_status = direct_command_RDSR.data;
    (void)(read_status);
    /*----------------------------------------------------------*/

    /*---------------- Read configure register(RDCR) (15h) ---------------*/
    spi_flash_direct_transfer_t direct_command_RDCR = {0};
    direct_command_RDCR.command        = 0x15U;
    direct_command_RDCR.command_length = 1U;
    direct_command_RDCR.data_length    = 1U;

    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command_RDCR, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    handle_error(err);
    uint32_t read_configure = direct_command_RDCR.data;
    (void)(read_configure);
    /*--------------------------------------------------------------*/

    /*--------------- Enable QPI mode(EQIO) (35h)---------------*/
    spi_flash_direct_transfer_t direct_command_EQIO = {0};
    direct_command_EQIO.command        = 0x35U;
    direct_command_EQIO.command_length = 1U;

    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command_EQIO, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
    handle_error(err);
    /*----------------------------------------------------------*/

    /* 4S-4S-4S protocol settings. */
    R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_4S_4S_4S);

    /*--------------- Read status register(RDSR) (05h) ---------------*/
    spi_flash_direct_transfer_t direct_command_RDSR_2 = {0};
    direct_command_RDSR_2.command        = 0x05U;
    direct_command_RDSR_2.command_length = 1U;
    direct_command_RDSR_2.data_length    = 1U;

    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command_RDSR_2, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    handle_error(err);
    read_status = direct_command_RDSR_2.data;
    /*----------------------------------------------------------*/

    /*---------------- Read configure register(RDCR) (15h) ---------------*/
    spi_flash_direct_transfer_t direct_command_RDCR_2 = {0};
    direct_command_RDCR_2.command        = 0x15U;
    direct_command_RDCR_2.command_length = 1U;
    direct_command_RDCR_2.data_length    = 1U;

    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command_RDCR_2, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    handle_error(err);
    read_configure = direct_command_RDCR_2.data;
    /*--------------------------------------------------------------*/

#elif (FW_UP_FLASH_TYPE == 1)
    /*---------------- Read status register2(RDSR2) (35h) ---------------*/
    spi_flash_direct_transfer_t direct_command_RDSR2 = {0};
    direct_command_RDSR2.command        = 0x35U;
    direct_command_RDSR2.command_length = 1U;
    direct_command_RDSR2.data_length    = 1U;

    fsp_err_t err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command_RDSR2, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    handle_error(err);
    uint32_t read_status2 = direct_command_RDSR2.data;

    /*---------------- Write enable (06h)----------------*/
    spi_flash_direct_transfer_t qspi_test_direct_transfer_write_enable = {0};
    qspi_test_direct_transfer_write_enable.command        = 0x06;
    qspi_test_direct_transfer_write_enable.command_length = 1U;
    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &qspi_test_direct_transfer_write_enable, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);

    /*---------------- Write Status Register-2 (31h) (set QE bit 1) ----------------*/
    spi_flash_direct_transfer_t direct_transfer_register_write = {0};
    direct_transfer_register_write.command              = 0x31;
    direct_transfer_register_write.data                 = 0x02 | read_status2;
    direct_transfer_register_write.command_length       = 1U;
    direct_transfer_register_write.data_length          = 1U;

    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_transfer_register_write, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
    handle_error(err);

    /* Wait for status register to update. */
    spi_flash_status_t status_write;
    do
    {
        (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_write);
    } while (true == status_write.write_in_progress);

    /* 1S-4S-4S protocol settings. */
    R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_4S_4S);

#endif
} /* End of qspi_set_Quad_Mode_enable */

/***********************************************************************************************************************
 * Function Name: qspi_set_Quad_Mode_disable
 * Description  : Disable QPI mode
 * Arguments    : non
 * Return Value : non
 ***********************************************************************************************************************/
void qspi_set_Quad_Mode_disable(void)
{
#if (FW_UP_FLASH_TYPE == 0)
    /*--------------- Reset QPI(RSTQIO) (F5h)---------------*/
    spi_flash_direct_transfer_t direct_command_RSTQIO = {0};
    direct_command_RSTQIO.command        = 0xF5U;
    direct_command_RSTQIO.command_length = 1U;

    fsp_err_t err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command_RSTQIO, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
    handle_error(err);
    /*----------------------------------------------------------*/

    /* 1S-1S-1S protocol settings. */
    R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);

    /*--------------- Read status register (05h) ---------------*/
    spi_flash_direct_transfer_t direct_command = {0};
    direct_command.command        = 0x05U;
    direct_command.command_length = 1U;
    direct_command.data_length    = 1U;

    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    handle_error(err);
    /*----------------------------------------------------------*/

    /*---------------- Read configure register (15h) ---------------*/
    direct_command.command        = 0x15U;
    direct_command.command_length = 1U;
    direct_command.data_length    = 1U;

    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    handle_error(err);
    /*--------------------------------------------------------------*/

#elif (FW_UP_FLASH_TYPE == 1)
    /* 1S-1S-1S protocol settings. */
    R_XSPI_QSPI_SpiProtocolSet(&g_qspi0_ctrl, SPI_FLASH_PROTOCOL_1S_1S_1S);

    /*---------------- Read status register2(RDSR2) (35h) ---------------*/
    spi_flash_direct_transfer_t direct_command_RDSR2 = {0};
    direct_command_RDSR2.command        = 0x35U;
    direct_command_RDSR2.command_length = 1U;
    direct_command_RDSR2.data_length    = 1U;
    fsp_err_t err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_command_RDSR2, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
    handle_error(err);
    uint32_t read_status = direct_command_RDSR2.data;

    /*---------------- Write enable (06h) ----------------*/
    spi_flash_direct_transfer_t qspi_test_direct_transfer_write_enable = {0};
    qspi_test_direct_transfer_write_enable.command        = 0x06;
    qspi_test_direct_transfer_write_enable.command_length = 1U;
    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &qspi_test_direct_transfer_write_enable, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);

    /*---------------- Write Status Register-2 (31h) (set QE bit 0) ----------------*/
    spi_flash_direct_transfer_t direct_transfer_register_write = {0};
    direct_transfer_register_write.command              = 0x31;
    direct_transfer_register_write.data                 = read_status & 0xfd;
    direct_transfer_register_write.command_length       = 1U;
    direct_transfer_register_write.data_length          = 1U;
    err = R_XSPI_QSPI_DirectTransfer(&g_qspi0_ctrl, &direct_transfer_register_write, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
    handle_error(err);

    /* Wait for status register to update. */
    spi_flash_status_t status_write;
    do
    {
        (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_write);
    } while (true == status_write.write_in_progress);

#endif

} /* End of qspi_set_Quad_Mode_disable */
#endif
