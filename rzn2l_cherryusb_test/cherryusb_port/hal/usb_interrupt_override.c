/**
 * @file usb_interrupt_override.c
 * @brief USB中断向量覆盖
 */

#include "bsp_api.h"
#include "bsp_irq.h"
#include "usb_dc_rzn2l.h"
#include "usb_log.h"

/**
 * @brief USB FS中断处理函数
 * @note 此函数由FSP中断向量表调用，向量号285
 *       直接调用CherryUSB中断处理，FSP框架已处理上下文保存/恢复
 */
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
