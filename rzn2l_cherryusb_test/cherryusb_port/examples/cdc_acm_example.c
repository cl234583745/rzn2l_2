/**
 * @file cdc_acm_example.c
 * @brief CDC ACM - 回环echo + 性能基准测试
 *
 * 协议：
 *   "#TX N\r\n"    发送N字节测试数据给PC (如 "#TX 65536\r\n")
 *   "#ECHO\r\n"    回到回环模式
 *   其他数据        echo回PC
 *
 * PC端测试 (Python):
 *   ser.write(b'#TX 131072\r\n')
 *   data = ser.read(131072)  # 接收测试数据
 *   查看设备调试串口的吞吐量报告
 */

#include "cdc_acm_example.h"
#include "cherryusb_config.h"
#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "bsp_api.h"
#include <string.h>
#include <stdio.h>

/* ---- RZ/N2L 全局计数器 (Cortex-R52 无 SysTick) ---- */
static uint64_t gsc_get(void)
{
    return (((uint64_t)R_GSC->CNTCVU << 32) | R_GSC->CNTCVL);
}

static uint32_t gsc_to_ms(uint64_t ticks)
{
    return (uint32_t)(ticks / (BSP_GLOBAL_SYSTEM_COUNTER_CLOCK_HZ / 1000));
}

struct usbd_interface g_cdc_acm_intf;
struct usbd_endpoint g_cdc_acm_ep_in;
struct usbd_endpoint g_cdc_acm_ep_out;
struct usbd_endpoint g_cdc_acm_ep_int;

/* ---- 数据缓冲 ---- */
static uint8_t g_rx_buf[CDC_ACM_RX_BUFFER_SIZE];
static uint8_t g_tx_buf[CDC_ACM_TX_BUFFER_SIZE];

/* ---- 标志 ---- */
static volatile bool g_rx_done;
static volatile bool g_tx_done;
static volatile uint32_t g_rx_bytes;

/* ---- 基准测试 ---- */
static uint8_t  g_bm_pattern[64];   /* pattern填充数据 */
static uint32_t g_bm_total;         /* 剩余待发字节 */
static uint32_t g_bm_sent;          /* 已发字节 */
static uint64_t g_bm_start_ticks;   /* 开始时刻(GSC) */

/* ---- 回调 ---- */
static void ep_in_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep; (void)nbytes;
    g_tx_done = true;
}

static void ep_out_cb(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep;
    g_rx_bytes = nbytes;
    g_rx_done = true;
}

static void cdc_send_notify(void);

/* ---- 初始化 ---- */
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

    for (int i = 0; i < 64; i++) g_bm_pattern[i] = (uint8_t)i;

    return 0;
}

/* ---- 简易 atoi ---- */
static uint32_t mini_atoi(const uint8_t *p, uint32_t len)
{
    uint32_t v = 0;
    while (len-- && *p >= '0' && *p <= '9') {
        v = v * 10 + (*p++ - '0');
    }
    return v;
}

/* ---- 主处理 ---- */
void cdc_acm_process(void)
{
    /* RX完成 */
    if (g_rx_done) {
        g_rx_done = false;

        if (g_rx_bytes >= 4 && memcmp(g_rx_buf, "#TX ", 4) == 0) {
            /* 吞吐量测试: #TX N */
            g_bm_total = mini_atoi(g_rx_buf + 4, g_rx_bytes - 4);
            if (g_bm_total > 0) {
                g_bm_sent = 0;
                g_bm_start_ticks = gsc_get();
                uint32_t n = (g_bm_total > 64) ? 64 : g_bm_total;
                cdc_acm_send(g_bm_pattern, n);
                g_bm_total -= n;
                g_bm_sent += n;
            }
        } else if (g_rx_bytes >= 5 && memcmp(g_rx_buf, "#ECHO", 5) == 0) {
            /* 回到回环模式 */
            g_bm_total = 0;
        } else if (g_rx_bytes > 0 && g_rx_bytes <= CDC_ACM_TX_BUFFER_SIZE) {
            /* 回环：收到"AT"开头的modem命令时回"OK"，否则原样echo */
            if (g_rx_bytes >= 2 && memcmp(g_rx_buf, "AT", 2) == 0) {
                static const char ok[] = "\r\nOK\r\n";
                cdc_acm_send((uint8_t *)ok, sizeof(ok) - 1);
            } else {
                /* echo前发EP3通知让Windows发起IN URB */
                cdc_send_notify();
                memcpy(g_tx_buf, g_rx_buf, g_rx_bytes);
                cdc_acm_send(g_tx_buf, g_rx_bytes);
            }
        }

        /* 非benchmark模式：重装接收。benchmark模式在发送完成后重装 */
        if (g_bm_total == 0 && g_bm_sent == 0) {
            usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, g_rx_buf, CDC_ACM_RX_BUFFER_SIZE);
        }
    }

    /* TX完成 */
    if (g_tx_done) {
        g_tx_done = false;

        if (g_bm_total > 0) {
            uint32_t n = (g_bm_total > 64) ? 64 : g_bm_total;
            cdc_acm_send(g_bm_pattern, n);
            g_bm_total -= n;
            g_bm_sent += n;
        } else if (g_bm_sent > 0) {
            uint64_t ticks = gsc_get() - g_bm_start_ticks;
            uint32_t ms = gsc_to_ms(ticks);
            if (ms == 0) ms = 1;
            uint32_t bps  = g_bm_sent * 1000 / ms;
            uint32_t kbps = bps / 1024;
            printf("\r\n#BM RESULT: %lu B / %lu ms = %lu KB/s\r\n",
                   g_bm_sent, ms, kbps);
            g_bm_sent = 0;
            usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, g_rx_buf, CDC_ACM_RX_BUFFER_SIZE);
        } else {
            usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, g_rx_buf, CDC_ACM_RX_BUFFER_SIZE);
        }
    }
}

/* ---- API ---- */
static void cdc_send_notify(void)
{
    uint8_t notify[10] = {
        0xA1, 0x20,           /* bmReqType + bNotification = SERIAL_STATE */
        0x00, 0x00,           /* wValue */
        0x00, 0x00,           /* wIndex = interface 0 */
        0x02, 0x00,           /* wLength = 2 */
        0x03, 0x00            /* Data: DCD=1, DSR=1 */
    };
    int ret = usbd_ep_start_write(0, CDC_ACM_EP_INT_ADDR, notify, 10);
    printf("#NOTIFY: ret=%d\r\n", ret);
}

// 复写CherryUSB弱函数: DTR变化时发通知
void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid; (void)intf;
    if (dtr) {
        cdc_send_notify();
    }
}

void cdc_acm_start_read(void)
{
    usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, g_rx_buf, CDC_ACM_RX_BUFFER_SIZE);
}

int cdc_acm_send(uint8_t *data, uint32_t len)
{
    if (!data || !len) return -1;
    g_tx_done = false;
    return usbd_ep_start_write(0, CDC_ACM_EP_IN_ADDR, data, len);
}

int cdc_acm_recv(uint8_t *data, uint32_t len)
{
    if (!data || !len) return -1;
    g_rx_done = false;
    return usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, data, len);
}

bool cdc_acm_is_rx_complete(void) { return g_rx_done; }
bool cdc_acm_is_tx_complete(void) { return g_tx_done; }
uint32_t cdc_acm_get_rx_len(void) { return g_rx_bytes; }
