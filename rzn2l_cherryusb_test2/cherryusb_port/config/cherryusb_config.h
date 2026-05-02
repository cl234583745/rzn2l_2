/**
 * @file cherryusb_config.h
 * @brief CherryUSB配置文件
 * @author CherryUSB Port
 * @date 2026-04-15
 */

#ifndef _CHERRYUSB_CONFIG_H_
#define _CHERRYUSB_CONFIG_H_

/* ========== USB版本配置 ========== */
#define USB_VERSION                     0x0200  /* USB 2.0 */

/* ========== 设备描述符配置 ========== */
#define USB_DEV_VID                     0x045B  /* Vendor ID (Renesas) */
#define USB_DEV_PID                     0x5539  /* Product ID */
#define USB_DEV_CLASS                   0x02    /* CDC Class */
#define USB_DEV_SUBCLASS                0x00
#define USB_DEV_PROTOCOL                0x00
#define USB_DEV_MAX_PACKET              64      /* EP0最大包长 */
#define USB_DEV_CONFIG_NUM              1       /* 配置数量 */
#define USB_DEV_STRING_NUM              6       /* 字符串描述符数量 */

/* ========== 端点配置 ========== */
#define USB_EP_NUM_MAX                  9       /* 最大端点数 */

/* ========== CDC ACM配置 ========== */
#define CDC_ACM_EP_IN_ADDR              0x81    /* IN端点地址 */
#define CDC_ACM_EP_OUT_ADDR             0x02    /* OUT端点地址 */
#define CDC_ACM_EP_INT_ADDR             0x83    /* 中断端点地址 */

#define CDC_ACM_EP_IN_MAX_PACKET        64      /* IN最大包长 */
#define CDC_ACM_EP_OUT_MAX_PACKET       64      /* OUT最大包长 */
#define CDC_ACM_EP_INT_MAX_PACKET       16      /* 中断最大包长 */
#define CDC_ACM_EP_INT_INTERVAL         10      /* 中断间隔(ms) */

/* ========== CDC ACM模式 ========== */
#define CDC_ACM_DEFAULT_MODE           0       /* 0=ECHO(默认), 1=SPEED_TEST, 运行时可切换 */

/* ========== 缓冲区配置 ========== */
#define CDC_ACM_RX_BUFFER_SIZE          2048    /* 接收缓冲区大小(CherryUSB官方测速用2048) */
#define CDC_ACM_TX_BUFFER_SIZE          2048    /* 发送缓冲区大小(CherryUSB官方测速用2048) */

/* ========== 回调配置 ========== */
#define USB_ENABLE_CALLBACK             1       /* 使能回调机制 */

/* ========== 调试配置 ========== */
#define USB_DEBUG_ENABLE                1       /* 启用调试输出 */

/* ========== 性能配置 ========== */
#define USB_DMA_ENABLE                  0       /* 禁用DMA(使用PIO) */
#define USB_DOUBLE_BUFFER_ENABLE        0       /* 禁用双缓冲 */

#endif /* _CHERRYUSB_CONFIG_H_ */
