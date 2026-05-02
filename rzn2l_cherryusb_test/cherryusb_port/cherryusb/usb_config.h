/**
 * @file usb_config.h
 * @brief CherryUSB配置文件 - RZ/N2L适配
 * @author CherryUSB Port
 * @date 2026-04-16
 *
 * @note 此文件放在cherryusb根目录,供CherryUSB核心使用
 */

#ifndef _USB_CONFIG_H_
#define _USB_CONFIG_H_

/* ========== USB设备配置 ========== */
#define CONFIG_USBDEV_MAX_BUS           1       /* 最大USB总线数 */
#define CONFIG_USBDEV_EP_NUM            9       /* 最大端点数 */
#define CONFIG_USBDEV_MSC_BLOCK_SIZE    512     /* MSC块大小 */

/* ========== 缓冲区配置 ========== */
#define CONFIG_USBDEV_EP0_BUFFER_SIZE   64      /* EP0缓冲区大小 */
#define CONFIG_USBDEV_REQUEST_BUFFER_SIZE 256   /* 请求缓冲区大小 */

#include "log.h"

/* ========== 调试配置 ========== */
/* 注意: USB中断内不要使用日志打印，避免与串口中断优先级冲突 */
#define CONFIG_USBDEV_DEBUG_LEVEL       1       /* 调试级别: 0=无, 1=错误, 2=警告, 3=信息 */
#define CONFIG_USB_DBG_LEVEL            1       /* 日志级别: 0=无, 1=错误, 2=警告, 3=信息 */
#define CONFIG_USB_PRINTF(...)          printf(__VA_ARGS__)  /* USB日志输出 */

/* ========== 性能配置 ========== */
//#define CONFIG_USBDEV_EP0_THREAD        0       /* EP0线程(裸机=0) */
#define CONFIG_USBDEV_EP0_STACKSIZE     1024    /* EP0线程栈大小 */
#define CONFIG_USBDEV_EP0_PRIO          5       /* EP0线程优先级 */

/* ========== 内存配置 ========== */
#define CONFIG_USB_ALIGN_SIZE           4       /* 对齐大小 */
#define CONFIG_USB_MEM_SECTION          ""      /* 内存段 */
#define USB_NOCACHE_RAM_SECTION         __attribute__((section(".nocache")))  /* NOCACHE段 */

/* ========== 缓冲区长度配置 ========== */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN  256   /* 请求缓冲区长度 */

/* ========== CDC ACM配置 ========== */
#define CONFIG_USBDEV_CDC_ACM_MAX_NUM   1       /* CDC ACM实例数 */

/* ========== OS抽象层配置(裸机) ========== */
#define CONFIG_USB_OSAL                 0       /* 0=裸机, 1=RTOS */

#endif /* _USB_CONFIG_H_ */
