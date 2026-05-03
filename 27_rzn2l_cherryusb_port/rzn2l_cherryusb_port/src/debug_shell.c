/**
 * @file debug_shell.c
 * @brief Debug UART command shell
 *
 * Uses FSP UART async read + callback to receive chars.
 * Buffers until Enter, then parses: "echo"/"e", "speed"/"s"
 */

#include "debug_shell.h"
#include "cdc_acm_example.h"
#include "hal_data.h"
#include "log.h"
#include <string.h>
#include <stdio.h>

#define CURRENT_LOG_LEVEL   LOG_LEVEL_INFO

#define CMD_BUF_SIZE 32

static char s_cmd_buf[CMD_BUF_SIZE];
static volatile uint8_t s_cmd_len;
static volatile bool s_cmd_ready;
static volatile bool s_skip_lf;

static uint8_t s_rx_byte;

void debug_shell_init(void)
{
    s_cmd_len = 0;
    s_cmd_ready = false;
    s_skip_lf = false;
    g_uart0.p_api->read(g_uart0.p_ctrl, &s_rx_byte, 1);
}

void debug_shell_poll(void)
{
    if (!s_cmd_ready) return;
    s_cmd_ready = false;

    uint8_t len = s_cmd_len;
    s_cmd_len = 0;

    s_cmd_buf[len] = '\0';
    if (strcmp(s_cmd_buf, "echo") == 0 || strcmp(s_cmd_buf, "e") == 0) {
        cdc_acm_switch_mode(CDC_ACM_MODE_ECHO);
    } else if (strcmp(s_cmd_buf, "speed") == 0 || strcmp(s_cmd_buf, "s") == 0) {
        cdc_acm_switch_mode(CDC_ACM_MODE_SPEED_TEST);
    } else {
        LOG_INFO("Unknown cmd: %s  (echo/speed)\r\n", s_cmd_buf);
    }
}

void debug_shell_uart_callback(uart_callback_args_t *p_args)
{
    if (p_args->event == UART_EVENT_RX_COMPLETE) {
        char ch = (char)s_rx_byte;

        if (ch == '\r' || ch == '\n') {
            if (s_skip_lf) {
                s_skip_lf = false;
            } else if (s_cmd_len > 0) {
                s_cmd_ready = true;
                s_skip_lf = true;
            }
        } else {
            s_skip_lf = false;
            if (ch == 0x7F || ch == 0x08) {
                if (s_cmd_len > 0) s_cmd_len--;
            } else if (s_cmd_len < CMD_BUF_SIZE - 1) {
                s_cmd_buf[s_cmd_len++] = ch;
            }
        }

        g_uart0.p_api->read(g_uart0.p_ctrl, &s_rx_byte, 1);
    }
}
