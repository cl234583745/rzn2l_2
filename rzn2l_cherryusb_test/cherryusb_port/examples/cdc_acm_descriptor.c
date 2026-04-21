/**
 * @file cdc_acm_descriptor.c
 * @brief CDC ACM USB描述符定义
 * @author CherryUSB Port
 * @date 2026-04-16
 */

#include "cdc_acm_descriptor.h"
#include "cherryusb_config.h"
#include "usbd_core.h"  /* 引入struct usb_descriptor定义 */

/* ========== 设备描述符 ========== */
const uint8_t cdc_acm_device_descriptor[] = {
    USB_DT_DEVICE_SIZE,              /* bLength */
    USB_DT_DEVICE,                   /* bDescriptorType */
    0x00, 0x02,                      /* bcdUSB = 2.00 */
    0x02,                            /* bDeviceClass = CDC */
    0x00,                            /* bDeviceSubClass */
    0x00,                            /* bDeviceProtocol */
    USB_DEV_MAX_PACKET,              /* bMaxPacketSize0 */
    (USB_DEV_VID & 0xFF), (USB_DEV_VID >> 8),        /* idVendor */
    (USB_DEV_PID & 0xFF), (USB_DEV_PID >> 8),        /* idProduct */
    0x00, 0x01,                      /* bcdDevice = 1.00 */
    0x01,                            /* iManufacturer */
    0x02,                            /* iProduct */
    0x03,                            /* iSerialNumber */
    USB_DEV_CONFIG_NUM               /* bNumConfigurations */
};

/* ========== 配置描述符 ========== */
#define CDC_ACM_TOTAL_LEN  (USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE + \
                            USB_DT_CS_INTERFACE_SIZE + USB_DT_CS_INTERFACE_SIZE + \
                            USB_DT_CS_INTERFACE_SIZE + USB_DT_CS_INTERFACE_SIZE + \
                            USB_DT_ENDPOINT_SIZE + USB_DT_INTERFACE_SIZE + \
                            USB_DT_ENDPOINT_SIZE + USB_DT_ENDPOINT_SIZE)

const uint8_t cdc_acm_config_descriptor[] = {
    /* Configuration Descriptor */
    USB_DT_CONFIG_SIZE,              /* bLength */
    USB_DT_CONFIG,                   /* bDescriptorType */
    (CDC_ACM_TOTAL_LEN & 0xFF), (CDC_ACM_TOTAL_LEN >> 8), /* wTotalLength */
    0x02,                            /* bNumInterfaces */
    0x01,                            /* bConfigurationValue */
    0x00,                            /* iConfiguration */
    0x80,                            /* bmAttributes: Bus-powered */
    0x32,                            /* bMaxPower = 100mA */

    /* Interface Association Descriptor (IAD) */
    0x08,                            /* bLength */
    0x0B,                            /* bDescriptorType: IAD */
    0x00,                            /* bFirstInterface */
    0x02,                            /* bInterfaceCount */
    0x02,                            /* bFunctionClass: CDC */
    0x02,                            /* bFunctionSubClass */
    0x01,                            /* bFunctionProtocol */
    0x00,                            /* iFunction */

    /* Interface 0: CDC Control Interface */
    USB_DT_INTERFACE_SIZE,           /* bLength */
    USB_DT_INTERFACE,                /* bDescriptorType */
    0x00,                            /* bInterfaceNumber */
    0x00,                            /* bAlternateSetting */
    0x01,                            /* bNumEndpoints */
    0x02,                            /* bInterfaceClass: CDC */
    0x02,                            /* bInterfaceSubClass: ACM */
    0x01,                            /* bInterfaceProtocol: AT Commands */
    0x00,                            /* iInterface */

    /* Header Functional Descriptor */
    0x05,                            /* bLength */
    0x24,                            /* bDescriptorType: CS_INTERFACE */
    0x00,                            /* bDescriptorSubtype: HEADER */
    0x10, 0x01,                      /* bcdCDC = 1.10 */

    /* Call Management Functional Descriptor */
    0x05,                            /* bLength */
    0x24,                            /* bDescriptorType: CS_INTERFACE */
    0x01,                            /* bDescriptorSubtype: CALL_MANAGEMENT */
    0x00,                            /* bmCapabilities */
    0x01,                            /* bDataInterface */

    /* ACM Functional Descriptor */
    0x04,                            /* bLength */
    0x24,                            /* bDescriptorType: CS_INTERFACE */
    0x02,                            /* bDescriptorSubtype: ACM */
    0x02,                            /* bmCapabilities: Support Set_Line_Coding, Set_Control_Line_State */

    /* Union Functional Descriptor */
    0x05,                            /* bLength */
    0x24,                            /* bDescriptorType: CS_INTERFACE */
    0x06,                            /* bDescriptorSubtype: UNION */
    0x00,                            /* bMasterInterface */
    0x01,                            /* bSlaveInterface */

    /* Endpoint 3: Interrupt IN */
    USB_DT_ENDPOINT_SIZE,            /* bLength */
    USB_DT_ENDPOINT,                 /* bDescriptorType */
    CDC_ACM_EP_INT_ADDR,             /* bEndpointAddress */
    0x03,                            /* bmAttributes: Interrupt */
    (CDC_ACM_EP_INT_MAX_PACKET & 0xFF), (CDC_ACM_EP_INT_MAX_PACKET >> 8), /* wMaxPacketSize */
    CDC_ACM_EP_INT_INTERVAL,         /* bInterval */

    /* Interface 1: CDC Data Interface */
    USB_DT_INTERFACE_SIZE,           /* bLength */
    USB_DT_INTERFACE,                /* bDescriptorType */
    0x01,                            /* bInterfaceNumber */
    0x00,                            /* bAlternateSetting */
    0x02,                            /* bNumEndpoints */
    0x0A,                            /* bInterfaceClass: CDC Data */
    0x00,                            /* bInterfaceSubClass */
    0x00,                            /* bInterfaceProtocol */
    0x00,                            /* iInterface */

    /* Endpoint 1: Bulk OUT */
    USB_DT_ENDPOINT_SIZE,            /* bLength */
    USB_DT_ENDPOINT,                 /* bDescriptorType */
    CDC_ACM_EP_OUT_ADDR,             /* bEndpointAddress */
    0x02,                            /* bmAttributes: Bulk */
    (CDC_ACM_EP_OUT_MAX_PACKET & 0xFF), (CDC_ACM_EP_OUT_MAX_PACKET >> 8), /* wMaxPacketSize */
    0x00,                            /* bInterval */

    /* Endpoint 2: Bulk IN */
    USB_DT_ENDPOINT_SIZE,            /* bLength */
    USB_DT_ENDPOINT,                 /* bDescriptorType */
    CDC_ACM_EP_IN_ADDR,              /* bEndpointAddress */
    0x02,                            /* bmAttributes: Bulk */
    (CDC_ACM_EP_IN_MAX_PACKET & 0xFF), (CDC_ACM_EP_IN_MAX_PACKET >> 8), /* wMaxPacketSize */
    0x00                             /* bInterval */
};

/* ========== 字符串描述符 ========== */
const char *cdc_acm_string_descriptor[] = {
    (const char []){ 0x09, 0x04 },                    /* LangID = 0x0409: English */
    "Renesas Electronics",                            /* iManufacturer */
    "RZ/N2L CDC ACM Device",                          /* iProduct */
    "12345678",                                       /* iSerialNumber */
    "CDC ACM Interface",                              /* iInterface */
    "CDC ACM Data Interface"                          /* iInterface */
};

/* ========== USB描述符结构 ========== */

/**
 * @brief 设备描述符回调
 */
static const uint8_t *cdc_acm_device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return cdc_acm_device_descriptor;
}

/**
 * @brief 配置描述符回调
 */
static const uint8_t *cdc_acm_config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return cdc_acm_config_descriptor;
}

/**
 * @brief 字符串描述符回调
 */
static const char *cdc_acm_string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if(index < (sizeof(cdc_acm_string_descriptor) / sizeof(const char *))) {
        return cdc_acm_string_descriptor[index];
    }
    return NULL;
}

const struct usb_descriptor cdc_acm_descriptor = {
    .device_descriptor_callback = cdc_acm_device_descriptor_callback,
    .config_descriptor_callback = cdc_acm_config_descriptor_callback,
    .device_quality_descriptor_callback = NULL,
    .other_speed_descriptor_callback = NULL,
    .string_descriptor_callback = cdc_acm_string_descriptor_callback,
    .msosv1_descriptor = NULL,
    .msosv2_descriptor = NULL,
    .webusb_url_descriptor = NULL,
    .bos_descriptor = NULL
};
