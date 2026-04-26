/**
 * @file usb_interrupt_override.c
 * @brief USB中断向量覆盖
 */

#include "bsp_api.h"
#include "bsp_irq.h"
#include "usb_dc_rzn2l.h"
#include "usb_log.h"

void usr_usbfs_interrupt_handler(void)
{
    USBD_IRQHandler(0);
}

void usb_interrupt_init(void)
{
    IRQn_Type usb_irq = (IRQn_Type)285;

    R_BSP_IrqDisable(usb_irq);

    R_BSP_IrqClearPending(usb_irq);

    R_BSP_IrqDetectTypeSet(usb_irq, 0);

    R_BSP_IrqCfg(usb_irq, 12, NULL);

    R_BSP_IrqEnable(usb_irq);
}
