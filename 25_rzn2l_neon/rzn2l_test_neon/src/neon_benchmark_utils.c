/**
 * @file neon_benchmark_utils.c
 * @brief NEON性能测试工具函数
 * 
 * 本文件提供NEON性能测试的基础工具函数，包括：
 * - 高精度计时器（基于GSC硬件计数器）
 * - 格式化UART输出（支持可变参数）
 * 
 * @note 修复记录：
 *       - 2026-04-11: 修复uart_printf可变参数传递bug
 *       - 添加stdarg.h支持，使用vprintf正确处理可变参数
 */

#include "hal_data.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>  // 可变参数支持：va_list, va_start, va_end

/* UART输出宏定义 */
#ifndef UART_PRINTF
#define UART_PRINTF printf
#endif

/* 
 * 全局时间变量（volatile防止编译器优化）
 * 用于记录性能测试的开始和结束时间
 */
static volatile uint32_t g_start_time;  ///< 测试开始时间戳
static volatile uint32_t g_end_time;    ///< 测试结束时间戳

/**
 * @brief 开始性能计时
 * 
 * 使用GSC（General System Controller）硬件计数器记录开始时间
 * GSC计数器提供高精度时间测量（25MHz时钟）
 * 
 * @note __DMB()确保内存屏障，防止指令重排序影响计时精度
 */
void neon_benchmark_start(void) {
    __DMB();  // 数据内存屏障，确保所有内存操作完成
    g_start_time = R_GSC->CNTCVL;  // 读取GSC计数器低32位
}

/**
 * @brief 结束性能计时并返回耗时
 * 
 * @return uint32_t 耗时（GSC时钟周期数）
 * 
 * @note 计算方式：end_time - start_time
 *       实际时间(秒) = cycles / GSC_CLOCK_FREQ
 */
uint32_t neon_benchmark_end(void) {
    __DMB();  // 数据内存屏障，确保所有内存操作完成
    g_end_time = R_GSC->CNTCVL;  // 读取GSC计数器低32位
    return g_end_time - g_start_time;  // 返回耗时周期数
}

/**
 * @brief 格式化UART输出函数
 * 
 * 支持printf风格的格式化输出，用于性能测试结果打印
 * 
 * @param fmt 格式化字符串
 * @param ... 可变参数列表
 * @return int 输出字符数（与printf一致）
 * 
 * @note 实现说明：
 *       - 使用标准C库的vprintf处理可变参数
 *       - va_list/va_start/va_end是标准可变参数处理机制
 *       - 修复前：直接调用UART_PRINTF(fmt)导致可变参数丢失
 *       - 修复后：使用vprintf正确传递所有参数
 * 
 * @example
 *   uart_print("Result: %d, Value: %f\r\n", count, value);
 */
int neon_benchmark_utils_uart_printf(const char *fmt, ...) {
    va_list args;       // 声明可变参数列表
    va_start(args, fmt); // 初始化可变参数，fmt是最后一个固定参数
    int ret = vprintf(fmt, args);  // 使用vprintf处理格式化输出
    va_end(args);       // 清理可变参数
    return ret;
}
