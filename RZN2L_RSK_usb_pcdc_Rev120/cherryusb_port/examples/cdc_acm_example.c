/**
 * @file cdc_acm_example.c
 * @brief CDC ACM示例实现 - CherryUSB
 */

#include "cdc_acm_example.h"
#include "cherryusb_config.h"
#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include <string.h>

struct usbd_interface g_cdc_acm_intf;
struct usbd_endpoint g_cdc_acm_ep_in;
struct usbd_endpoint g_cdc_acm_ep_out;
struct usbd_endpoint g_cdc_acm_ep_int;

static volatile bool g_rx_complete = false;
static volatile bool g_tx_complete = false;
static volatile uint32_t g_rx_len = 0;

static uint8_t g_rx_buffer[CDC_ACM_RX_BUFFER_SIZE];
static uint8_t g_tx_buffer[CDC_ACM_TX_BUFFER_SIZE];

static void cdc_acm_ep_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    g_tx_complete = true;
    (void)nbytes;
}

static void cdc_acm_ep_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    g_rx_complete = true;
    g_rx_len = nbytes;
}

int cdc_acm_init(void)
{
    g_cdc_acm_ep_in.ep_addr = CDC_ACM_EP_IN_ADDR;
    g_cdc_acm_ep_in.ep_cb = cdc_acm_ep_in_callback;
    usbd_add_endpoint(0, &g_cdc_acm_ep_in);

    g_cdc_acm_ep_out.ep_addr = CDC_ACM_EP_OUT_ADDR;
    g_cdc_acm_ep_out.ep_cb = cdc_acm_ep_out_callback;
    usbd_add_endpoint(0, &g_cdc_acm_ep_out);

    g_cdc_acm_ep_int.ep_addr = CDC_ACM_EP_INT_ADDR;
    g_cdc_acm_ep_int.ep_cb = NULL;
    usbd_add_endpoint(0, &g_cdc_acm_ep_int);

    usbd_cdc_acm_init_intf(0, &g_cdc_acm_intf);
    usbd_add_interface(0, &g_cdc_acm_intf);

    return 0;
}

void cdc_acm_process(void)
{
    if (g_rx_complete) {
        if (g_rx_len > 0 && g_rx_len <= CDC_ACM_TX_BUFFER_SIZE) {
            memcpy(g_tx_buffer, g_rx_buffer, g_rx_len);
            cdc_acm_send(g_tx_buffer, g_rx_len);
        }
        g_rx_complete = false;
    }

    if (g_tx_complete) {
        usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, g_rx_buffer, CDC_ACM_RX_BUFFER_SIZE);
        g_tx_complete = false;
    }
}

int cdc_acm_send(uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0) return -1;
    g_tx_complete = false;
    return usbd_ep_start_write(0, CDC_ACM_EP_IN_ADDR, data, len);
}

int cdc_acm_recv(uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0) return -1;
    g_rx_complete = false;
    return usbd_ep_start_read(0, CDC_ACM_EP_OUT_ADDR, data, len);
}

bool cdc_acm_is_rx_complete(void)
{
    return g_rx_complete;
}

bool cdc_acm_is_tx_complete(void)
{
    return g_tx_complete;
}

uint32_t cdc_acm_get_rx_len(void)
{
    return g_rx_len;
}
