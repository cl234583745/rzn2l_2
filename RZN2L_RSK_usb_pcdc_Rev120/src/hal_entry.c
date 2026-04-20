#include "hal_data.h"
#include "usb_dc_rzn2l.h"
#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "usb_interrupt_override.h"
#include "cdc_acm_example.h"

#include "log.h"

#define CURRENT_LOG_LEVEL   LOG_LEVEL_DEBUG

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
BSP_PLACE_IN_SECTION(".warm_start");
FSP_CPP_FOOTER


volatile bool uartTxCompleteFlg = 0;
/*
    R_SCI_B_UART_Open(&g_uart9_ctrl, &g_uart9_cfg);
    printf("\nToolchain ver:%s\n", __VERSION__);
    printf("date:%s\ntime:%s\nfile:%s\nfunc:%s,line:%d\nhello world!\n", __DATE__, __TIME__, __FILE__, __FUNCTION__, __LINE__);
 */
#define PRINTF  1   //only need change g_uart9_ctrl, uartcallback=NULL
#define POLLING_REG 1
#if PRINTF
#include <stdio.h>
static void USR_SCI_UART_Write (uart_ctrl_t * const p_api_ctrl, uint8_t const * const p_src, uint32_t const bytes);
static void USR_SCI_UART_Write (uart_ctrl_t * const p_api_ctrl, uint8_t const * const p_src, uint32_t const bytes)
{
    uint32_t i;
#if defined(R_SCI_B_UART_CFG_H_)  //RA6T2 RA8T2
    sci_b_uart_instance_ctrl_t * p_ctrl = (sci_b_uart_instance_ctrl_t *) p_api_ctrl;
#else
    sci_uart_instance_ctrl_t * p_ctrl = (sci_uart_instance_ctrl_t *) p_api_ctrl;
#endif//#if defined(R_SCI_B_UART_CFG_H_)
    uint8_t *data  = (uint8_t *)p_src;
    for(i = 0; i < bytes; i++)
    {
#if defined(R_SCI_B_UART_CFG_H_)  //RA6T2 RA8T2
        p_ctrl->p_reg->TDR_b.TDAT = *data;
        while(p_ctrl->p_reg->CSR_b.TDRE == 0);
#else

#if defined(_RENESAS_RA_)  //RA excluded RA6T2 RA8T2
        p_ctrl->p_reg->TDR_b.TDR = *data;
        while(p_ctrl->p_reg->SSR_b.TDRE == 0);
#elif defined(_RENESAS_RZN_) || defined(_RENESAS_RZT_) //RZN RZT
        p_ctrl->p_reg->TDR_b.TDAT = *data;
        while(p_ctrl->p_reg->CSR_b.TDRE == 0);

#endif//#if defined(_RENESAS_RA_)
#endif//#if defined(R_SCI_B_UART_CFG_H_)
        data++;
    }
}
#if defined __GNUC__ && !defined __clang__  //e2+gcc
int _write(int fd, char *pBuffer, int size);
int _write(int fd, char *pBuffer, int size)
{
    (void)fd;
#if POLLING_REG
    USR_SCI_UART_Write(&g_uart0_ctrl, (uint8_t *)pBuffer, (uint32_t)size);
#else
    uartTxCompleteFlg = 0;
    g_uart0.p_api->write(g_uart0.p_ctrl, pBuffer, size);
    while(!uartTxCompleteFlg);
    uartTxCompleteFlg = 0;
#endif
    return size;
}
#elif defined __llvm__ && defined __clang__ //e2+llvm
int user_write(char ptr, FILE* file)
{
    (void)(file);
    USR_SCI_UART_Write(&g_uart9_ctrl, (uint8_t * const)(&ptr), 1U);
    return 0;
}
/* Redirect sdtio as per https://github.com/picolibc/picolibc/blob/main/doc/os.md */
static FILE __stdio = FDEV_SETUP_STREAM(user_write, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);

#elif defined __ICCARM__    //iar
#include <yfuns.h>
#if __VER__ < 8000000
  _STD_BEGIN
#endif
  #pragma module_name = "?__write"
size_t __write(int handle, const unsigned char * buffer, size_t size)
{
      USR_SCI_UART_Write(&g_uart9_ctrl, (uint8_t *)pBuffer, (uint32_t)size);
}
#if __VER__ < 8000000
  _STD_END
#endif
#else   //keil
int fputc(int ch, FILE *f)
{
    (void)f;
    USR_SCI_UART_Write(&g_uart9_ctrl, (uint8_t *)&ch, 1);

    return ch;
}
#endif//#if defined __GNUC__ && !defined __clang__
#endif//#if PRINTF





extern struct usbd_interface g_cdc_acm_intf;
extern struct usbd_endpoint g_cdc_acm_ep_in;
extern struct usbd_endpoint g_cdc_acm_ep_out;
extern struct usbd_endpoint g_cdc_acm_ep_int;

static void cdc_acm_event_handler(uint8_t busid, uint8_t event);

void hal_entry(void)
{
    __asm volatile ("cpsie i");


#if PRINTF
    __enable_irq();

    g_uart0.p_api->open(g_uart0.p_ctrl, g_uart0.p_cfg);
    g_uart0.p_api->write(g_uart0.p_ctrl, (uint8_t *)"g_uart0.p_api->open\n",strlen("g_uart0.p_api->open\n"));
#if 1//!POLLING_REG
    while(!uartTxCompleteFlg);
    uartTxCompleteFlg = 0;
#endif
    LOG_INFO("date:%s\ntime:%s\nfile:%s\nfunc:%s,line:%d\nhello world!\n", __DATE__, __TIME__, __FILE__, __FUNCTION__, __LINE__);

    float PI = 3.1415926f;
    LOG_INFO("PI=%f\n", PI);
    LOG_INFO("%s\n", FSP_VERSION_BUILD_STRING);

    R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
#endif




    usbd_initialize(0, R_USBF_BASE, cdc_acm_event_handler);

    extern const struct usb_descriptor cdc_acm_descriptor;
    usbd_desc_register(0, &cdc_acm_descriptor);

    cdc_acm_init();

    usb_interrupt_init();

    while(1)
    {
        cdc_acm_process();
        __WFI();
    }
}

static void cdc_acm_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;

    switch(event) {
        case USBD_EVENT_INIT:
            break;
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_CONFIGURED:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SET_INTERFACE:
            break;
        default:
            break;
    }
}

void R_BSP_WarmStart (bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_POST_C == event)
    {
        R_IOPORT_Open(&g_ioport_ctrl, &g_bsp_pin_cfg);
    }
}

void user_uart_callback(uart_callback_args_t *p_args)
{
    if(p_args->event == UART_EVENT_TX_COMPLETE)//
    {
        uartTxCompleteFlg = 1;
    }
}
