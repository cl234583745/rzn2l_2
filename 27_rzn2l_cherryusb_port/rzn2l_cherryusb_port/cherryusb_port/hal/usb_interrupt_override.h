// Copyright (c) 2026 chenkaka. MIT licensed.
/**
 * @file usb_interrupt_override.h
 * @brief USB中断覆盖接口
 */

#ifndef _USB_INTERRUPT_OVERRIDE_H_
#define _USB_INTERRUPT_OVERRIDE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 自定义USB中断处理函数
 */
void usr_usbfs_interrupt_handler(void);

/**
 * @brief USB中断初始化
 */
void usb_interrupt_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _USB_INTERRUPT_OVERRIDE_H_ */
