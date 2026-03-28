/*
 * crc32_table.c
 *
 *  Created on: 2026年2月11日
 *      Author: Jerry.Chen
 */
#ifndef CRC32_TABLE_H
#define CRC32_TABLE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// CRC上下文结构体（相当于C++类）
typedef struct {
    uint32_t init;          // 初始值
    uint32_t poly;          // 多项式
    uint32_t table[256];    // CRC表
} CRC_Context;

/**
 * @brief 初始化CRC上下文
 * @param ctx CRC上下文指针
 * @param init 初始值（默认：0xFFFFFFFF）
 * @param poly 多项式（默认：0xEDB88320，即标准CRC32多项式）
 */
void CRC_Init(CRC_Context* ctx, uint32_t init, uint32_t poly);

/**
 * @brief 计算数据的CRC32值
 * @param ctx CRC上下文指针
 * @param data 数据指针
 * @param length 数据长度（字节）
 * @return 计算出的CRC32值
 */
uint32_t CRC_Calculate(const CRC_Context* ctx, const char* data, int length);

/**
 * @brief 计算数据的CRC32值（使用字节指针）
 * @param ctx CRC上下文指针
 * @param data 数据指针（字节）
 * @param length 数据长度（字节）
 * @return 计算出的CRC32值
 */
uint32_t CRC_CalculateBytes(const CRC_Context* ctx, const uint8_t* data, int length);

/**
 * @brief 快速计算CRC32（使用默认参数）
 * @param data 数据指针
 * @param length 数据长度
 * @return 计算出的CRC32值
 *
 * 使用默认参数：init=0xFFFFFFFF, poly=0xEDB88320
 */
uint32_t CRC_CalculateFast(const void* data, int length);

/**
 * @brief 快速计算CRC32（指定初始值和多项式）
 * @param data 数据指针
 * @param length 数据长度
 * @param init 初始值
 * @param poly 多项式
 * @return 计算出的CRC32值
 */
uint32_t CRC_CalculateEx(const void* data, int length, uint32_t init, uint32_t poly);

#ifdef __cplusplus
}
#endif

#endif // CRC32_TABLE_H
