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
 * File Name    : r_fw_up_rz_feature.h
 * Version      : 1.00
 * Description  : Firmware update faeture definition file
 ***********************************************************************************************************************/
/**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 31.04.2023 1.00     First Release for RZ/T2L
 ***********************************************************************************************************************/
#ifdef FW_PARSE_SREC
#ifndef R_FW_UP_RZ_FEATURE_H_
#define R_FW_UP_RZ_FEATURE_H_

/***********************************************************************************************************************
 Macro definitions
 ***********************************************************************************************************************/
/* S-Flash type to be updated */
/* 0 - MX25U51245G (RSK RZ/N2L,RZ/T2M), 1 - AT25SF128A (RSK RZ/T2L) */
#if defined(BOARD_RZT2L_RSK)
#define FW_UP_FLASH_TYPE        (1)
#else
#define FW_UP_FLASH_TYPE        (0)
#endif

/* xSPI uint number */
/* 0 - xSPI0 (RSK RZ/N2L,RZ/T2M), 1 - xSPI1 (RSK RZ/T2L)*/
#if defined(BOARD_RZT2L_RSK)
#define FW_UP_QSPI_UNIT         (1)
#else
#define FW_UP_QSPI_UNIT         (0)
#endif

#endif /* R_FW_UP_RZ_FEATURE_H_ */
#endif
