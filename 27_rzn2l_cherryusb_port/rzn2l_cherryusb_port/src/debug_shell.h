/**
 * @file debug_shell.h
 * @brief Debug UART command shell - FSP UART async read + callback
 */

#ifndef _DEBUG_SHELL_H_
#define _DEBUG_SHELL_H_

#include <stdint.h>
#include "hal_data.h"

#ifdef __cplusplus
extern "C" {
#endif

void debug_shell_init(void);
void debug_shell_poll(void);
void debug_shell_uart_callback(uart_callback_args_t *p_args);

#ifdef __cplusplus
}
#endif

#endif
