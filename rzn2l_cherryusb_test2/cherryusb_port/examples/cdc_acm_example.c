/**
 * @file cdc_acm_example.c
 * @brief CDC ACM - 回环echo + 速度测试 (参考CherryUSB官方模板)
 *
 * 速度测试 (CherryUSB官方方式):
 *   DTR=1 → 设备持续发2048字节给PC
 *   DTR=0 → 停止发送，回到回环模式
 *
 * PC端测试:
 *   python test_cdc_speed.py COM11
 */

#include "cdc_acm_example.h"
#include "cherryusb_config.h"
#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "bsp_api.h"
#include "log.h"
#include <string.h>
#include <stdio.h>

#define CURRENT_LOG_LEVEL   LOG_LEVEL_INFO

struct usbd_interface g_cdc_acm_intf;
struct usbd_endpoint g_cdc_acm_ep_in;
struct usbd_endpoint g_cdc_acm_ep_out;
struct usbd_endpoint g_cdc_acm_ep_int;

/* ---- 数据缓冲 (参考CherryUSB模板, 2048字节) ---- */
static uint8_t g_rx_buf[CDC_ACM_RX_BUFFER_SIZE];
static uint8_t g_tx_buf[CDC_ACM_TX_BUFFER_SIZE];

/* ---- 标志 ---- */
static volatile bool g_tx_busy;
static volatile bool g_rx_done;
static volatile uint32_t g_rx_bytes;

volatile uint8_t g_cdc_mode = CDC_ACM_DEFAULT_MODE;
static volatile uint8_t g_dtr_enable;

static uint64_t gsc_get(void)
{
    return (((uint64_t)R_GSC->CNTCVU << 32) | R_GSC->CNTCVL);
}

static uint32_t gsc_to_ms(uint64_t ticks)
{
    return (uint32_t)(ticks / (BSP_GLOBAL_SYSTEM_COUNTER_CLOCK_HZ / 1000));
}

static uint32_t g_echo_total_bytes;
static uint32_t g_echo_total_ms;

static void ep_in_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep; (void)nbytes;
    g_tx_busy = false;
}

static void ep_out_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep;
    g_rx_bytes = nbytes;
    g_rx_done = true;
}

static void print_mode_info(void)
{
    LOG_INFO("\r\n=== CDC ACM Mode: %s ===\r\n",
             g_cdc_mode == CDC_ACM_MODE_ECHO ? "ECHO" : "SPEED_TEST");
    LOG_INFO("  ECHO:       send data via COM -> MCU echoes back\r\n");
    LOG_INFO("  SPEED_TEST: DTR=1 -> MCU sends data continuously\r\n");
    LOG_INFO("              Use: python test_cdc_speed.py COMx\r\n");
    LOG_INFO("  Debug UART cmd: \"echo\" or \"speed\" + Enter\r\n\r\n");
}

int cdc_acm_init(void)
{
    g_cdc_acm_ep_in.ep_addr  = CDC_ACM_EP_IN_ADDR;
    g_cdc_acm_ep_in.ep_cb    = ep_in_cb;
    usbd_add_endpoint(0, &g_cdc_acm_ep_in);

    g_cdc_acm_ep_out.ep_addr = CDC_ACM_EP_OUT_ADDR;
    g_cdc_acm_ep_out.ep_cb   = ep_out_cb;
    usbd_add_endpoint(0, &g_cdc_acm_ep_out);

    g_cdc_acm_ep_int.ep_addr = CDC_ACM_EP_INT_ADDR;
    g_cdc_acm_ep_int.ep_cb   = NULL;
    usbd_add_endpoint(0, &g_cdc_acm_ep_int);

    usbd_cdc_acm_init_intf(0, &g_cdc_acm_intf);
    usbd_add_interface(0, &g_cdc_acm_intf);

    /* 填充测试数据 */
    for (int i = 0; i < CDC_ACM_TX_BUFFER_SIZE; i++)
        g_tx_buf[i] = (uint8_t)(i & 0xFF);

    return 0;
}

void cdc_acm_print_banner(void)
{
    print_mode_info();
}

void cdc_acm_switch_mode(uint8_t mode)
{
    g_cdc_mode = mode;
    g_dtr_enable = 0;
    print_mode_info();
}

void cdc_acm_process(void)
{
    /* RX完成: echo回PC */
    if (g_rx_done) {
        g_rx_done = false;

        uint32_t rx_len = g_rx_bytes;
        memcpy(g_tx_buf, g_rx_buf, rx_len);

        uint64_t t0 = gsc_get();

        g_tx_busy = true;
        usbd_ep_start_write(0, CDC_ACM_EP_IN_ADDR, g_tx_buf, rx_len);
        while (g_tx_busy) {}

        uint32_t ms = gsc_to_ms(gsc_get() - t0);
        g_echo_total_bytes += rx_len * 2;
        g_echo_total_ms += ms;
        if (g_echo_total_ms >= 2000) {
            LOG_INFO("ECHO: %lu B / %lu ms = %lu KB/s\r\n",
                     g_echo_total_bytes, g_echo_total_ms,
                     g_echo_total_bytes * 1000 / g_echo_total_ms / 1024);
            g_echo_total_bytes = 0;
            g_echo_total_ms = 0;
        }

        usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, g_rx_buf, CDC_ACM_RX_BUFFER_SIZE);
    }

    /* DTR=1时持续发送 (速度测试模式) */
    if (g_dtr_enable) {
        g_tx_busy = true;
        usbd_ep_start_write(0, CDC_ACM_EP_IN_ADDR, g_tx_buf, CDC_ACM_TX_BUFFER_SIZE);
        while (g_tx_busy) {}
    }
}

/* ---- DTR回调 (CherryUSB弱函数) ---- */
void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid; (void)intf;
    if (dtr && g_cdc_mode == CDC_ACM_MODE_SPEED_TEST) {
        g_dtr_enable = 1;
        LOG_INFO("[SPEED TEST] Sending data continuously. Use: python test_cdc_speed.py COMx\r\n");
    } else {
        g_dtr_enable = 0;
    }
}

/* ---- 开始读取 ---- */
void cdc_acm_start_read(void)
{
    usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, g_rx_buf, CDC_ACM_RX_BUFFER_SIZE);
}

int cdc_acm_send(uint8_t *data, uint32_t len)
{
    if (!data || !len) return -1;
    g_tx_busy = true;
    return usbd_ep_start_write(0, CDC_ACM_EP_IN_ADDR, data, len);
}

int cdc_acm_recv(uint8_t *data, uint32_t len)
{
    if (!data || !len) return -1;
    return usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, data, len);
}

bool cdc_acm_is_rx_complete(void) { return g_rx_done; }
bool cdc_acm_is_tx_complete(void) { return !g_tx_busy; }
uint32_t cdc_acm_get_rx_len(void) { return g_rx_bytes; }
