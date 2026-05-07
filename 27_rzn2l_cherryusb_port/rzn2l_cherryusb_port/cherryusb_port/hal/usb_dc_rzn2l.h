// Copyright (c) 2026 chenkaka. MIT licensed.
/**
 * @file usb_dc_rzn2l.h
 * @brief RZ/N2L USB Device Controller HAL for CherryUSB
 * @author CherryUSB Port
 * @date 2026-04-16
 */

#ifndef _USB_DC_RZN2L_H_
#define _USB_DC_RZN2L_H_

#include "../../cherryusb/common/usb_dc.h"

#ifdef __cplusplus
extern "C" {
#endif

void USBD_IRQHandler(uint8_t busid);

#ifdef __cplusplus
}
#endif

#endif /* _USB_DC_RZN2L_H_ */
