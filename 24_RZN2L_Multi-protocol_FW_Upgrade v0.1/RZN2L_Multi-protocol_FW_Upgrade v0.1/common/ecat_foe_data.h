/*
 * ecat_foe_data.h
 *
 *  Created on: 2026年2月12日
 *      Author: Jerry.Chen
 */
#include "flash_config.h"

#ifndef COMMON_ECAT_FOE_DATA_H_
#define COMMON_ECAT_FOE_DATA_H_


#define    HEADER_PARAMS_APP        9
#define    HEADER_PARAMS_LENS       0x4c

#pragma pack(1)
typedef struct
{
    uint8_t header_app[HEADER_PARAMS_APP];
    uint32_t header_len;
    uint8_t header_reserved[HEADER_PARAMS_LENS - HEADER_PARAMS_APP - 4];

    union
    {
        uint32_t  dword[4];
        uint16_t  word[8];
        uint8_t   byte[16];
    };

} app_header_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
    app_header_t current_header;

    uint32_t current_addr;
    uint32_t current_crc;
    uint32_t recv_offset;
    uint32_t write_offset;
    uint8_t Read_Buffer[FW_UP_PAGE_SIZE];

} ota_handle_t;
#pragma pack()


#endif /* COMMON_ECAT_FOE_DATA_H_ */
