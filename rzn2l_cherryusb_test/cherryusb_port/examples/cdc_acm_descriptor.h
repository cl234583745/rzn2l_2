/**
 * @file cdc_acm_descriptor.h
 * @brief CDC ACM USB描述符定义头文件
 * @author CherryUSB Port
 * @date 2026-04-16
 */

#ifndef _CDC_ACM_DESCRIPTOR_H_
#define _CDC_ACM_DESCRIPTOR_H_

#include "usbd_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* USB描述符常量定义 */
#define USB_DT_DEVICE                   0x01
#define USB_DT_CONFIG                   0x02
#define USB_DT_STRING                   0x03
#define USB_DT_INTERFACE                0x04
#define USB_DT_ENDPOINT                 0x05
#define USB_DT_DEVICE_QUALIFIER         0x06
#define USB_DT_OTHER_SPEED_CONFIG       0x07
#define USB_DT_INTERFACE_POWER          0x08
#define USB_DT_INTERFACE_ASSOCIATION    0x0B
#define USB_DT_CS_INTERFACE             0x24
#define USB_DT_CS_ENDPOINT              0x25

/* 描述符大小 */
#define USB_DT_DEVICE_SIZE              18
#define USB_DT_CONFIG_SIZE              9
#define USB_DT_INTERFACE_SIZE           9
#define USB_DT_ENDPOINT_SIZE            7
#define USB_DT_CS_INTERFACE_SIZE        5

/* 外部声明 */
extern const uint8_t cdc_acm_device_descriptor[];
extern const uint8_t cdc_acm_config_descriptor[];
extern const char *cdc_acm_string_descriptor[];
extern const struct usb_descriptor cdc_acm_descriptor;

#ifdef __cplusplus
}
#endif

#endif /* _CDC_ACM_DESCRIPTOR_H_ */
