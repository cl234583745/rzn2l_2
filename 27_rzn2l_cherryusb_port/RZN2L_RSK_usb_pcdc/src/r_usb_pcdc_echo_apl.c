/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2014(2018) Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************/

#include "hal_data.h"
#include "r_usb_basic.h"
#include "r_usb_basic_cfg.h"
#include "r_usb_pcdc_cfg.h"
#include "r_usb_pcdc_apl.h"
#include "r_usb_pcdc_api.h"
#include "r_usb_typedef.h"
#include "r_usb_extern.h"
#include <stdio.h>
#include "log.h"

#if USB_CFG_DTC == USB_CFG_ENABLE
#include "r_dtc_rx_if.h"
#endif

#if USB_CFG_DMA == USB_CFG_ENABLE
#endif

#if (BSP_CFG_RTOS == 2)
#include "inc/r_usb_rtos_apl.h"
#endif

#ifdef USB_CFG_PCDC_USE
#if OPERATION_MODE == USB_ECHO

/*===========================================================================
 * 性能测试配置 - 可运行时切换 ECHO / 性能测试模式
 *===========================================================================*/
#define BENCH_BUF_SIZE      4096
#define GSC_HZ              BSP_GLOBAL_SYSTEM_COUNTER_CLOCK_HZ
#define BENCH_OUT_SIZE      (2 * 1024 * 1024)   /* 2MB for OUT test */
#define BENCH_IN_SIZE       (10 * 1024 * 1024)  /* 10MB for IN test  */

/* 模式定义 */
typedef enum {
    MODE_ECHO = 0,      /* 回环模式 (DTR=1) */
    MODE_BENCH_OUT,     /* PC->MCU 测试 (DTR=0) */
    MODE_BENCH_IN       /* MCU->PC 测试 (DTR=1 触发) */
} app_mode_t;

static volatile app_mode_t g_mode = MODE_ECHO;

/* 缓冲区 */
static uint8_t g_buf[DATA_LEN + 4];
static uint8_t g_bench_tx_buf[BENCH_BUF_SIZE];
static usb_pcdc_linecoding_t g_line_coding;

/* 测试统计 */
static volatile uint32_t g_rx_count = 0;
static volatile uint32_t g_tx_count = 0;
static volatile uint64_t g_time_start = 0;
static volatile uint64_t g_time_end = 0;

/* 传输状态 */
static volatile bool g_tx_busy = false;
static volatile bool g_rx_busy = false;

/* 描述符 */
extern uint8_t g_apl_device[];
extern uint8_t g_apl_configuration[];
extern uint8_t g_apl_hs_configuration[];
extern uint8_t g_apl_qualifier_descriptor[];
extern uint8_t *g_apl_string_table[];

/* const */ usb_descriptor_t g_usb_descriptor = {
    g_apl_device, g_apl_configuration, g_apl_hs_configuration,
    g_apl_qualifier_descriptor, g_apl_string_table, NUM_STRING_DESCRIPTOR
};

extern const usb_api_t g_usb_on_usb;

static uint64_t get_ticks(void) {
    return (((uint64_t)R_GSC->CNTCVU << 32) | R_GSC->CNTCVL);
}

static void bench_tx_pattern_init(void) {
    for (int i = 0; i < BENCH_BUF_SIZE; i++) {
        g_bench_tx_buf[i] = (uint8_t)(i & 0xFF);
    }
}

/* 开始接收请求 */
static void start_read(void *buf, uint32_t len) {
    if (!g_rx_busy) {
        if (g_usb_on_usb.read(&g_basic0_ctrl, buf, len, USB_CLASS_PCDC) == FSP_SUCCESS) {
            g_rx_busy = true;
        }
    }
}

/* 开始发送请求 */
static void start_write(void *buf, uint32_t len) {
    if (!g_tx_busy) {
        if (g_usb_on_usb.write(&g_basic0_ctrl, buf, len, USB_CLASS_PCDC) == FSP_SUCCESS) {
            g_tx_busy = true;
        }
    }
}

/*===========================================================================
 * USB 事件处理
 *===========================================================================*/
void usb_basic_example(void) {
    usb_event_info_t event_info;
    usb_status_t event;

    bench_tx_pattern_init();

    g_usb_on_usb.open(&g_basic0_ctrl, &g_basic0_cfg);

    /* 打印 USB 配置信息 */
    LOG_INFO("[USB] Module: USB_IP%d\r\n", g_basic0_cfg.module_number);
    LOG_INFO("[USB] Mode: %s\r\n", (g_basic0_cfg.usb_mode == USB_MODE_PERI) ? "Peripheral" : "Host");
    LOG_INFO("[USB] Speed: %s\r\n",
           (g_basic0_cfg.usb_speed == USB_SPEED_HS) ? "High-Speed" :
           (g_basic0_cfg.usb_speed == USB_SPEED_FS) ? "Full-Speed" : "Low-Speed");
    LOG_INFO("[USB] DMA: %s\r\n", (USB_CFG_DMA == USB_CFG_ENABLE) ? "ON" : "OFF");

    /* 从设备描述符读取 VID/PID */
    extern uint8_t g_apl_device[];
    uint16_t vid = (uint16_t)g_apl_device[8] | ((uint16_t)g_apl_device[9] << 8);
    uint16_t pid = (uint16_t)g_apl_device[10] | ((uint16_t)g_apl_device[11] << 8);
    uint16_t bcd_usb = (uint16_t)g_apl_device[2] | ((uint16_t)g_apl_device[3] << 8);
    LOG_INFO("[USB] VID: 0x%04X, PID: 0x%04X\r\n", vid, pid);
    LOG_INFO("[USB] bcdUSB: 0x%04X\r\n", bcd_usb);
    LOG_INFO("[USB] BENCH_BUF_SIZE: %d\r\n", BENCH_BUF_SIZE);

    /* 启动第一次读取 */
    start_read(g_buf, DATA_LEN);

    while (1) {
        g_usb_on_usb.eventGet(&event_info, &event);

        switch (event) {

        case USB_STATUS_CONFIGURED:
            break;

        /* 收到数据 */
        case USB_STATUS_READ_COMPLETE:
            g_rx_busy = false;

            switch (g_mode) {
            case MODE_ECHO:
                /* 回环模式：收到什么就发回什么 */
                start_write(g_buf, event_info.data_size);
                break;

            case MODE_BENCH_OUT:
                /* OUT测试：只计数，不回传 */
                if (g_rx_count == 0) {
                    g_time_start = get_ticks();
                }
                g_rx_count += event_info.data_size;
                /* 继续接收 */
                start_read(g_buf, DATA_LEN);
                break;

            case MODE_BENCH_IN:
                /* IN测试：忽略收到的数据 */
                start_read(g_buf, DATA_LEN);
                break;
            }
            break;

        /* 发送完成 */
        case USB_STATUS_WRITE_COMPLETE:
            g_tx_busy = false;

            if (g_mode == MODE_ECHO) {
                /* 回环模式：发完后继续接收 */
                start_read(g_buf, DATA_LEN);
            }
            break;

        /* USB类请求 */
        case USB_STATUS_REQUEST:
            if (USB_PCDC_SET_LINE_CODING == (event_info.setup.request_type & USB_BREQUEST)) {
                g_usb_on_usb.periControlDataGet(&g_basic0_ctrl, (uint8_t *)&g_line_coding, LINE_CODING_LENGTH);
            } else if (USB_PCDC_GET_LINE_CODING == (event_info.setup.request_type & USB_BREQUEST)) {
                g_usb_on_usb.periControlDataSet(&g_basic0_ctrl, (uint8_t *)&g_line_coding, LINE_CODING_LENGTH);
            } else if (USB_PCDC_SET_CONTROL_LINE_STATE == (event_info.setup.request_type & USB_BREQUEST)) {
                g_usb_on_usb.periControlStatusSet(&g_basic0_ctrl, USB_SETUP_STATUS_ACK);

                /* DTR 信号控制模式切换 */
                uint8_t dtr = (event_info.setup.request_value & 0x0001);
                if (dtr == 0) {
                    /* DTR=0: 进入 OUT 测试模式 */
                    LOG_INFO("[c] OUT Test Start (PC->MCU)\r\n");
                    g_mode = MODE_BENCH_OUT;
                    g_rx_count = 0;
                    g_time_start = 0;
                    /* 确保有读取请求在运行 */
                    start_read(g_buf, DATA_LEN);
                } else {
                    /* DTR=1: 如果之前是OUT模式，打印结果并进入IN模式 */
                    if (g_mode == MODE_BENCH_OUT && g_time_start > 0) {
                        g_time_end = get_ticks();
                        uint32_t ms = (uint32_t)((g_time_end - g_time_start) / (GSC_HZ / 1000));
                        if (ms == 0) ms = 1;
                        float speed = (float)g_rx_count / 1048576.0f / ((float)ms / 1000.0f);
                        LOG_INFO("[c] OUT Speed: %.2f MB/s (%lu B in %lu ms)\r\n", speed, g_rx_count, ms);
                    }

                    /* 进入 IN 测试模式 */
                    LOG_INFO("[c] IN Test Start (MCU->PC)\r\n");
                    g_mode = MODE_BENCH_IN;
                    g_tx_count = 0;
                    g_time_start = get_ticks();
                    g_tx_busy = false;
                }
            } else {
                g_usb_on_usb.periControlStatusSet(&g_basic0_ctrl, USB_SETUP_STATUS_ACK);
            }
            break;

        case USB_STATUS_SUSPEND:
        case USB_STATUS_DETACH:
            break;
        default:
            break;
        }

        /*-------------------------------------------------------
         * 主循环：IN 测试发送逻辑
         * 只在 MODE_BENCH_IN 模式下运行
         *-------------------------------------------------------*/
        if (g_mode == MODE_BENCH_IN) {
            if (g_tx_count < BENCH_IN_SIZE) {
                /* 还有数据要发 */
                if (!g_tx_busy) {
                    uint32_t len = BENCH_BUF_SIZE;
                    if (g_tx_count + len > BENCH_IN_SIZE) {
                        len = BENCH_IN_SIZE - g_tx_count;
                    }
                    if (g_usb_on_usb.write(&g_basic0_ctrl, g_bench_tx_buf, len, USB_CLASS_PCDC) == FSP_SUCCESS) {
                        g_tx_busy = true;
                        g_tx_count += len;
                    }
                }
            } else {
                /* 发送完成 */
                if (!g_tx_busy) {
                    g_time_end = get_ticks();
                    uint32_t ms = (uint32_t)((g_time_end - g_time_start) / (GSC_HZ / 1000));
                    if (ms == 0) ms = 1;
                    float speed = (float)g_tx_count / 1048576.0f / ((float)ms / 1000.0f);
                    LOG_INFO("[c] IN Speed: %.2f MB/s (%lu B in %lu ms)\r\n", speed, g_tx_count, ms);

                    /* 回到 ECHO 模式 */
                    g_mode = MODE_ECHO;
                    start_read(g_buf, DATA_LEN);
                }
            }
        }
    }
}

#if (BSP_CFG_RTOS == 2)
void usb_apl_callback(usb_event_info_t *p_api_event, usb_hdl_t cur_task, usb_onoff_t usb_state) {
    (void)usb_state;
    (void)cur_task;
    USB_APL_SND_MSG(USB_APL_MBX, (usb_msg_t *)p_api_event);
}

usb_er_t usb_apl_rec_msg(uint8_t id, usb_msg_t **mess, usb_tm_t tm) {
    (void)tm;
    if (NULL == mess) return USB_APL_ERROR;
    QueueHandle_t handle = (*(g_apl_mbx_table[id]));
    *mess = NULL;
    if (pdTRUE == xQueueReceive(handle, (void *)mess, (portMAX_DELAY)) && NULL != (*mess))
        return USB_APL_OK;
    return USB_APL_ERROR;
}

usb_er_t usb_apl_snd_msg(uint8_t id, usb_msg_t *mess) {
    if (NULL == mess) return USB_APL_ERROR;
    QueueHandle_t handle = (*(g_apl_mbx_table[id]));
    if (pdTRUE == xQueueSend(handle, (const void *)&mess, (TickType_t)(0)))
        return USB_APL_OK;
    return USB_APL_ERROR;
}
#endif

#endif /* OPERATION_MODE == USB_ECHO */
#endif /* USB_CFG_PCDC_USE */
