#include "hal_data.h"

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event)
BSP_PLACE_IN_SECTION(".warm_start");
FSP_CPP_FOOTER


/*
    R_SCI_B_UART_Open(&g_uart9_ctrl, &g_uart9_cfg);
    printf("\nToolchain ver:%s\n", __VERSION__);
    printf("date:%s\ntime:%s\nfile:%s\nfunc:%s,line:%d\nhello world!\n", __DATE__, __TIME__, __FILE__, __FUNCTION__, __LINE__);
 */
#define PRINTF  1   //only need change g_uart9_ctrl, uartcallback=NULL
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
    USR_SCI_UART_Write(&g_uart0_ctrl, (uint8_t *)pBuffer, (uint32_t)size);

    return size;
}
#elif defined(__llvm__) && defined(__clang__) && !defined(__ARMCC_VERSION) //e2+llvm, exclude Keil
int user_write(char ptr, FILE* file)
{
    (void)(file);
    USR_SCI_UART_Write(&g_uart0_ctrl, (uint8_t * const)(&ptr), 1U);
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
      USR_SCI_UART_Write(&g_uart0_ctrl, (uint8_t *)buffer, (uint32_t)size);
}
#if __VER__ < 8000000
  _STD_END
#endif
#else   //keil
int fputc(int ch, FILE *f)
{
    (void)f;
    USR_SCI_UART_Write(&g_uart0_ctrl, (uint8_t *)&ch, 1);

    return ch;
}
#endif//#if defined __GNUC__ && !defined __clang__
#endif//#if PRINTF


extern void hal_entry(void);

void hal_entry(void)
{
    R_SCI_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
    printf("\nToolchain ver:%s\n", __VERSION__);
    printf("SystemCoreClock: %d Hz\n", SystemCoreClock);
    printf("date:%s\ntime:%s\nfile:%s\nfunc:%s,line:%d\nhello world!\n", __DATE__, __TIME__, __FILE__, __FUNCTION__, __LINE__);


    extern void neon_benchmark_main(void);
    neon_benchmark_main();

    while(1)
    {
        printf("running!\n");
        R_BSP_SoftwareDelay(1000, 1000);
    }
}

void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
    }

    if (BSP_WARM_START_POST_C == event)
    {
        R_IOPORT_Open (&g_ioport_ctrl, &g_bsp_pin_cfg);
    }
}

void user_uart_callback(uart_callback_args_t *p_args);
void user_uart_callback(uart_callback_args_t *p_args)
{
    (void)(p_args);
}
