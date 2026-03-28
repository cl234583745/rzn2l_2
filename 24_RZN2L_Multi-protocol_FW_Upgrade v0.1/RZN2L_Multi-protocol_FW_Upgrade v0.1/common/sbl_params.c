/*
 * sbl_params.c
 *
 *  Created on: 2026年2月12日
 *      Author: Jerry.Chen
 */

#include <stdio.h>
#include <stdint.h>
#include "sbl_params.h"
#include "flash_config.h"
#include "ecat_foe_data.h"
#include "crc32_table.h"
#include "hal_data.h"
#include "log.h"

#define CURRENT_LOG_LEVEL   LOG_LEVEL_DEBUG

/*
 * 读取BootBankParams，检查crc和与当前bank是否一致，不一致则恢复当前bank
 */
void checkAndUpdataBootBankParams(void)
{
    uint8_t structParams[FW_UP_WRITE_ATONCE_SIZE] = {0};
    uint16_t idx = 0;
    ota_handle_t curBankHdr = {0};

    // 1. 读取boot bank params
    uint8_t rdBootParams[FW_UP_WRITE_ATONCE_SIZE] = {0};
    memcpy(rdBootParams, (uint8_t *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET - FW_UP_BOOT_PARAMS_SIZE), sizeof(rdBootParams));
    LOG_DEBUG("read boot Params:");
    for(uint8_t i = 0; i < BOOT_PARAMS_LENS; i++)
    {
        LOG_DEBUG("%02X", rdBootParams[i]);
    }LOG_DEBUG("\n");

    uint32_t rdBootCrc = *(uint32_t*)(rdBootParams + BOOT_PARAMS_LENS - 4);
extern CRC_Context ctx;
    uint32_t calBootCrc = CRC_Calculate(&ctx, (char*)rdBootParams, BOOT_PARAMS_LENS - 4);
    LOG_DEBUG(" rdBootCrc:%lX\ncalBootCrc=%lX\n", rdBootCrc, calBootCrc);

    if(rdBootCrc != calBootCrc)
    {
        LOG_INFO("ERR!!!\n");

        // 2. 构造当前bank的boot bank params
#if (BANK == 0)
        memcpy(&curBankHdr.current_header, (uint8_t *)(FW_UP_BANK0_ADDR), sizeof(app_header_t));
#elif (BANK == 1)
        memcpy(&curBankHdr.current_header, (uint8_t *)(FW_UP_BANK1_ADDR), sizeof(app_header_t));
#endif

        memcpy(structParams + idx, curBankHdr.current_header.header_app, HEADER_PARAMS_APP);
        idx += HEADER_PARAMS_APP;

extern const uint32_t g_identify[4];
        memcpy(structParams + idx, curBankHdr.current_header.byte, sizeof(g_identify));
        idx += sizeof(g_identify);

        uint32_t crcBootParams = CRC_Calculate(&ctx, (char*)structParams, idx);
        memcpy(structParams + idx, (uint8_t*)&crcBootParams, sizeof(crcBootParams));
        idx += sizeof(crcBootParams);

        LOG_DEBUG("structParams:");
        for(uint8_t i = 0; i < BOOT_PARAMS_LENS; i++)
        {
            LOG_DEBUG("%02X", structParams[i]);
        }LOG_DEBUG("\n");

        // 3. erase BootBankParams

        R_XSPI_QSPI_Erase(&g_qspi0_ctrl, (uint8_t *)(FW_UP_BANK0_ADDR - FW_UP_BOOT_PARAMS_SIZE),(uint32_t)FW_UP_BOOT_PARAMS_SIZE);
        spi_flash_status_t status_erase;
        do{
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
        } while (true == status_erase.write_in_progress);

        // 4. write BootBankParams
        R_XSPI_QSPI_Write(&g_qspi0_ctrl, structParams,
                (uint8_t *)(FW_UP_BANK0_ADDR - FW_UP_BOOT_PARAMS_SIZE),
                                          (uint32_t)FW_UP_WRITE_ATONCE_SIZE);
        do{
            (void) R_XSPI_QSPI_StatusGet(&g_qspi0_ctrl, &status_erase);
        } while (true == status_erase.write_in_progress);

        LOG_INFO("write current bank to BootBankParams!!!\n");
    }
    else
    {
        LOG_INFO("BootBankParams check OK!!!\n");
    }

}

/*
 *
 */
void sblCheckBootParams(void)
{
    // 1. 读取boot bank params
    uint8_t rdBootParams[FW_UP_WRITE_ATONCE_SIZE] = {0};
    memcpy(rdBootParams, (uint8_t *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET - FW_UP_BOOT_PARAMS_SIZE), sizeof(rdBootParams));
    LOG_DEBUG("read boot Params:");
    for(uint8_t i = 0; i < BOOT_PARAMS_LENS; i++)
    {
        LOG_DEBUG("%02X", rdBootParams[i]);
    }LOG_DEBUG("\n");

    uint32_t rdBootCrc = *(uint32_t*)(rdBootParams + BOOT_PARAMS_LENS - 4);
extern CRC_Context ctx;
    uint32_t calBootCrc = CRC_Calculate(&ctx, (char*)rdBootParams, BOOT_PARAMS_LENS - 4);
    LOG_DEBUG(" rdBootCrc:%lX\ncalBootCrc=%lX\n", rdBootCrc, calBootCrc);

    if(rdBootCrc != calBootCrc)
    {
        LOG_INFO("ERR!!!\n");

#if APP1_ENABLE

        volatile uint32_t *pCheckCrc = (uint32_t *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET);    // Identify section address

        uint32_t crcCalRet = CRC_Calculate(&ctx, (char*)pCheckCrc, (int)((uint32_t*)((uint8_t*)pCheckCrc + APP1_HDR)) );
        uint32_t crcFlashData = *((uint32_t *)(FW_UP_BANK0_ADDR - FW_UP_MIRROR_OFFSET + (uint32_t)((uint32_t*)((uint8_t*)pCheckCrc + APP1_HDR) - 4)) );

        if(crcCalRet != crcFlashData)
        {
            LOG_DEBUG("APP1 BANK0 check crc NG, crcCalRet=%lX, crcFlashData=%lX\n", crcCalRet, crcFlashData);
            LOG_INFO("ready check  APP1 BANK1!!!\n");
        }
        else
        {
            LOG_INFO("APP1 BANK0 check crc OK, !!!\n");
            LOG_INFO("ready default BootParams to APP1 BANK0!!!\n");
        }

#endif

    }
    else
    {
        LOG_INFO("BootBankParams check OK!!!\n");
    }
}
