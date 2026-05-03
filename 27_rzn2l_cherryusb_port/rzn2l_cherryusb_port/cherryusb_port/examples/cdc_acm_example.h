/**
 * @file cdc_acm_example.h
 * @brief CDC ACM interface - runtime switchable echo / speed test
 */

#ifndef _CDC_ACM_EXAMPLE_H_
#define _CDC_ACM_EXAMPLE_H_

#include <stdint.h>
#include <stdbool.h>
#include "usbd_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CDC_ACM_MODE_ECHO       0
#define CDC_ACM_MODE_SPEED_TEST 1

extern struct usbd_interface g_cdc_acm_intf;
extern struct usbd_endpoint g_cdc_acm_ep_in;
extern struct usbd_endpoint g_cdc_acm_ep_out;
extern struct usbd_endpoint g_cdc_acm_ep_int;

extern volatile uint8_t g_cdc_mode;

int cdc_acm_init(void);
void cdc_acm_start_read(void);
void cdc_acm_process(void);
void cdc_acm_print_banner(void);
void cdc_acm_switch_mode(uint8_t mode);

int cdc_acm_send(uint8_t *data, uint32_t len);
int cdc_acm_recv(uint8_t *data, uint32_t len);
bool cdc_acm_is_rx_complete(void);
bool cdc_acm_is_tx_complete(void);
uint32_t cdc_acm_get_rx_len(void);

#ifdef __cplusplus
}
#endif

#endif
