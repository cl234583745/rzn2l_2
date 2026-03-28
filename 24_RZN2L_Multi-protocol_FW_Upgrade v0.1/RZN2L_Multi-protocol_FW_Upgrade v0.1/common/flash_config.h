/*
 * flash_config.h
 *
 *  Created on: 2026年2月5日
 *      Author: Jerry.Chen
 */

#ifndef FLASH_CONFIG_H_
#define FLASH_CONFIG_H_

#define APP1_ENABLE             1
#define APP2_ENABLE             0
#define APP3_ENABLE             0
#define APP4_ENABLE             0

#if APP1_ENABLE
/* Select BANK */
#if APP1_BANK == 0
#define BANK 0
#elif APP1_BANK == 1
#define BANK 1
#endif


#define FW_UP_BANK0_ADDR        (0x60100000)    /* BANK0 user applicaiotn */
#define FW_UP_BANK1_ADDR        (0x60200000)    /* BANK1 user applicaiotn */

#define FW_UP_BOOT_PARAMS_SIZE  (4 * 1024)      /* Sector size = 64 KB   */
#define FW_UP_SECTOR_SIZE       (64 * 1024)     /* Sector size = 64 KB   */
#define FW_UP_TOTAL_SIZE        (128 * 1024)    /* Sector size = 128 KB   */

/* Flash driver WRITE API restriction */
#define FW_UP_WRITE_ATONCE_SIZE (64) // Byte
#define FW_UP_PAGE_SIZE         (256)// page program

#define FW_UP_MIRROR_OFFSET     (0x20000000)    /* xSPI0 Mirror space minus offset  */

#define FW_UP_PACKAGE_SIZE      (116)// package send size

#define APP1_HDR                9//APP1BANK0


#define BOOT_PARAMS_LENS        29// (9+16+4)

#define app1_bank0_offset       0x0000005c// 0x4c+4x4
#define app1_bank1_offset       0x0000005c// 0x4c+4x4

#endif//#if APP1_ENABLE

#if APP2_ENABLE

#endif//#if APP2_ENABLE

#if APP3_ENABLE

#endif//#if APP3_ENABLE

#if APP4_ENABLE

#endif//#if APP4_ENABLE

#endif /* FLASH_CONFIG_H_ */
