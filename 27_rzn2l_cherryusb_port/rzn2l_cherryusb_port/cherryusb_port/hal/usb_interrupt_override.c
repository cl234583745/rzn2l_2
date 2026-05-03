/**
 * @file usb_interrupt_override.c
 * @brief USB中断处理 - 适配FSP框架
 */

#include "bsp_api.h"
#include "../../src/bsp/mcu/all/bsp_irq.h"
#include "usb_dc_rzn2l.h"
#include "usb_reg_rzn2l.h"
#include "usb_log.h"

/**
 * @brief USB FS中断处理函数（向量号285）
 * @note 由FSP中断向量表调用。函数名必须与g_vector_table[285]中的一致。
 */
void usr_usbfs_interrupt_handler(void)
{
    USBD_IRQHandler(0);
}

void usb_interrupt_init(void)
{
    IRQn_Type usb_irq = 285;
    
    /* 清除所有USB外设中断标志（防止旧中断影响） */
    R_USBF->INTSTS0 = 0;
    R_USBF->BRDYSTS = 0;
    R_USBF->BEMPSTS = 0;
    R_USBF->NRDYSTS = 0;
    
    /* 使能USB外设中断 - 包括控制传输阶段转换中断(CTRE) */
    R_USBF->INTENB0 = (USB_VBSE | USB_DVSE | USB_CTRE | USB_BRDYE | USB_BEMPE | USB_NRDYE | USB_RSME);
    R_USBF->NRDYENB = 0;
    
    /* 配置GIC：优先级12，电平触发 */
    R_BSP_IrqCfg(usb_irq, 12, NULL);
    R_BSP_IrqDetectTypeSet(usb_irq, 0);  // 0=电平触发
    R_BSP_IrqEnable(usb_irq);
    
    USB_LOG_INFO("USB: IRQ #%d enabled (level), INTENB0=0x%04X\n", 
                 usb_irq, R_USBF->INTENB0);
}
