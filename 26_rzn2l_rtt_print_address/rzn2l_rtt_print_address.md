二十六、RZN2L CR52 RTT打印移植与固定地址
===
[toc]

# 一、目的/概述

1、在RZN2L CR52内核上移植SEGGER RTT打印功能

2、解决RTT地址不固定的问题确保J-Link能稳定连接

3、强制放置到nocache SRAM地址保证数据一致性

# 二、问题背景

## 2.1 为什么需要nocache地址？

RZN2L使用的是Cortex-R52内核，有多块内存区域，其中部分使用了Cache：

| 内存区域 | 地址 | 是否Cache |
|----------|------|---------|
| ATCM | 0x00000000 | 无 |
| BTCM | 0x00100000 | 无 |
| SYSTEM_RAM_MIRROR | 0x30000000 | 有 |
| NONCACHE_BUFFER | 0x30160000 | 无（nocache） |

RTT需要放在nocache区域，否则J-Link读取时数据可能还在Cache里未刷到RAM。

## 2.2 地址不固定的问题

默认情况下，RTT的三个变量在`.noncache_buffer` section中按链接顺序排列：

```
.noncache_buffer
                0x30160000      0x4b8
                0x30160410                _SEGGER_RTT    ← 不是起始地址！
```

每次编译顺序不同，地址也会变化，导致J-Link无法稳定连接。

# 三、移植步骤

## 3.1 复制RTT文件到工程

从SEGGER官网下载RTT源码，复制以下文件到工程：

```
src/SEGGER_RTT/
├── SEGGER_RTT.c
├── SEGGER_RTT.h
├── SEGGER_RTT_Conf.h
└── SEGGER_RTT_printf.c
```

## 3.2 配置log.h

在`src/log.h`中配置打印方式：

```c
/*
 * 打印方式选择: 0=UART, 1=RTT
 */
#define PRINTF_METHOD  1

#if (PRINTF_METHOD == 1)
    #define USE_RTT_PRINT       1
    #define SEGGER_INDEX     (0)
    #include "SEGGER_RTT/SEGGER_RTT.h"
    #define printf(...) SEGGER_RTT_printf(SEGGER_INDEX, __VA_ARGS__)
#endif
```

## 3.3 修改SEGGER_RTT.c

修改`src/SEGGER_RTT/SEGGER_RTT.c`第274-276行，使用nocache section：

```c
#if SEGGER_RTT_CPU_CACHE_LINE_SIZE
  // 有cache的系统使用对齐
  SEGGER_RTT_CB _SEGGER_RTT __attribute__ ((section (".noncache_buffer"), aligned (SEGGER_RTT_CPU_CACHE_LINE_SIZE)));
  // ...
#else
  // 无cache的系统直接放到nocache段
  SEGGER_RTT_CB _SEGGER_RTT __attribute__ ((section (".noncache_buffer")));
  static char _acUpBuffer[BUFFER_SIZE_UP] __attribute__ ((section (".noncache_buffer")));
  static char _acDownBuffer[BUFFER_SIZE_DOWN] __attribute__ ((section (".noncache_buffer")));
#endif
```

## 3.4 修改链接脚本

在`script/fsp_xspi0_boot.ld`中固定顺序：

```ld
.noncache_buffer NONCACHE_BUFFER_START (NOLOAD) : AT (NONCACHE_BUFFER_START)
{
    . = ALIGN(32);
    _ncbuffer_start = .;

    KEEP(*(.noncache_buffer._SEGGER_RTT)))
    KEEP(*(.noncache_buffer._acUpBuffer)))
    KEEP(*(.noncache_buffer._acDownBuffer)))
    KEEP(*(.noncache_buffer*))

    _ncbuffer_end = .;
} > NONCACHE_BUFFER
```

## 3.5 定义起始标记

为了确保`_SEGGER_RTT`在section起始位置，需要添加一个起始标记：

```c
// 在RTT控制块前面添加一个标记变量
static char _SEGGER_RTT_Start[1] __attribute__ ((section (".noncache_buffer")));

SEGGER_RTT_CB _SEGGER_RTT __attribute__ ((section (".noncache_buffer"), aligned(32)));
static char _acUpBuffer[BUFFER_SIZE_UP] __attribute__ ((section (".noncache_buffer")));
static char _acDownBuffer[BUFFER_SIZE_DOWN] __attribute__ ((section (".noncache_buffer")));
```

链接脚本更新为：

```ld
KEEP(*(.noncache_buffer._SEGGER_RTT_Start)))
KEEP(*(.noncache_buffer._SEGGER_RTT)))
KEEP(*(.noncache_buffer._acUpBuffer)))
KEEP(*(.noncache_buffer._acDownBuffer)))
KEEP(*(.noncache_buffer*))
```

# 四、验证方法

## 4.1 查看map文件

编译后在map文件中确认地址：

```
.noncache_buffer
                0x30160000      0x4b8
                0x30160000                        . = ALIGN (0x20)
                0x30160000                        _ncbuffer_start = .
 *(.noncache_buffer._SEGGER_RTT_Start)
 *(.noncache_buffer._SEGGER_RTT)
 *(.noncache_buffer._acUpBuffer)
 *(.noncache_buffer._acDownBuffer)
 .noncache_buffer
                0x30160000      0x4b8 ./src/SEGGER_RTT/SEGGER_RTT.o
                0x30160000                _SEGGER_RTT    ← 固定在起始地址
                0x301604b8                        _ncbuffer_end = .
```

## 4.2 J-Link连接测试

在J-Link Commander中验证：

```
J-Link> exec device = R52
J-Link> mem 0x30160000, 4
```

# 五、常见问题

## 5.1 RTT不打印

检查项：

1. 是否正确包含`SEGGER_RTT.h`
2. 缓冲区是否放到nocache section
3. J-Link是否连接正确

## 5.2 地址不固定

原因：变量未放到相同的section或链接脚本ORDER不对

解决：按本文第3.5节添加起始标记

## 5.3 有cache和无cache的区别

| 配置 | 说明 |
|------|------|
| 有L1 Cache | 必须放nocache，否则数据不一致 |
| 无Cache | 可以放普通RAM，nocache可选 |

# 六、总结

| 步骤 | 关键操作 |
|------|---------|
| 1 | 复制RTT源文件到工程 |
| 2 | 配置log.h，使用RTT打印 |
| 3 | 修改SEGGER_RTT.c，放到noncache_buffer section |
| 4 | 添加起始标记_SEGGER_RTT_Start |
| 5 | 修改链接脚本，固定排列顺序 |
| 6 | 验证map文件确认地址固定 |

核心要点：**必须定义起始标记 + 链接脚本固定ORDER**，才能保证`_SEGGER_RTT`固定在0x30160000地址。

# 七、附录

## 测试环境

- **MCU**：Renesas RZN2L
- **内核**：Cortex-R52 @ 400MHz
- **编译器**：ARM GCC 13.3.1
- **IDE**：e2studio
- **J-Link**：V11

## 相关地址定义

| 符号 | 地址 | 说明 |
|------|------|------|
| NONCACHE_BUFFER_START | 0x30160000 | nocache区域起始 |
| NONCACHE_BUFFER_LENGTH | 0x00020000 | 128KB |
| _SEGGER_RTT | 0x30160000 | RTT控制块 |
| _acUpBuffer | 0x30160410 | 上行缓冲区 |
| _acDownBuffer | 0x30160810 | 下行缓冲区 |