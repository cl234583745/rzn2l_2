/**
 * @file cdc_acm_example.h
 * @brief CDC ACM示例接口
 */

#ifndef _CDC_ACM_EXAMPLE_H_
#define _CDC_ACM_EXAMPLE_H_

#include <stdint.h>
#include <stdbool.h>
#include "usbd_core.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct usbd_interface g_cdc_acm_intf;
extern struct usbd_endpoint g_cdc_acm_ep_in;
extern struct usbd_endpoint g_cdc_acm_ep_out;
extern struct usbd_endpoint g_cdc_acm_ep_int;

int cdc_acm_init(void);
void cdc_acm_start_read(void);
void cdc_acm_process(void);
int cdc_acm_send(uint8_t *data, uint32_t len);
int cdc_acm_recv(uint8_t *data, uint32_t len);
bool cdc_acm_is_rx_complete(void);
bool cdc_acm_is_tx_complete(void);
uint32_t cdc_acm_get_rx_len(void);

#ifdef __cplusplus
}
#endif

#endif
