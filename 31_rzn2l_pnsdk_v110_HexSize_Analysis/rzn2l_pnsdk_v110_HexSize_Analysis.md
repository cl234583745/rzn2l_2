三十一、RZ/N2L PN RT IRT SDK烧录文件膨胀分析与修复
===

[toc]

---

## 1. 问题现象

SDK v1.10 默认配置下，编译生成的 HEX 文件 **14MB**，添加 NOLOAD 后仅 **3MB**，差值 **11MB**。

| 文件 | 无 NOLOAD | 有 NOLOAD | 差值 |
|------|----------|----------|------|
| HEX | 14 MB | 3 MB | **11 MB** |

根本原因：`.sdram_nc`（~2MB）和 `.heap`（~1.94MB）两个 section 在链接脚本中**缺少 `(NOLOAD)` 属性**，导致其内容被写入输出文件。二进制 4MB 在 HEX 格式下膨胀为 11MB。

## 2. 根本原因

### 2.1 Section 类型决定是否输出

GNU 链接器根据**输入段类型**决定输出 section 类型：

| 输入段 | 输出类型 | 是否写入文件 |
|--------|---------|------------|
| `SHT_PROGBITS` | PROGBITS | 是 |
| `SHT_NOBITS` | NOBITS | 否 |

**规则**：只要有一个输入段是 PROGBITS，整个输出就是 PROGBITS。

### 2.2 各 section 分析

```
.bss 的输入：
  *(.bss*)      → NOBITS（零初始化）
  *(COMMON)     → NOBITS
  → 输出 NOBITS → objcopy 跳过 ✓

.heap 的输入：
  *(.heap)      → PROGBITS（C 库 heap 池数组）
  → 只要有一个 PROGBITS → 输出 PROGBITS → objcopy 写入 ✗

.sdram_nc 的输入：
  *(uncached_mem*) → PROGBITS（用户定义的变量）
  → 只要有一个 PROGBITS → 输出 PROGBITS → objcopy 写入 ✗
```

### 2.3 为什么 .bss 不需要 NOLOAD

`.bss` 是 ELF 标准 section，编译器保证其所有输入段都是 `SHT_NOBITS`，链接器自动输出 `NOBITS`，`objcopy` 自然跳过。

`.heap` 和 `.sdram_nc` 包含 PROGBITS 输入段，必须用 `(NOLOAD)` 强制降级为 `NOBITS`。

## 3. 解决方案

### 3.1 推荐方案：修改链接脚本

在 `.ld` 文件中添加 `(NOLOAD)`，去掉冗余的 `AT`：

```ld
// 修改前：
.sdram_nc SDRAM_NC_START : AT (SDRAM_NC_START) { ... } > SDRAM_NC_SPACE
.heap HEAP_START : AT (HEAP_START) { ... } > HEAP_SPACE

// 修改后：
.sdram_nc (NOLOAD) : { ... } > SDRAM_NC_SPACE
.heap (NOLOAD) : { ... } > HEAP_SPACE
```

### 3.2 Workaround：修改 objcopy 命令

通过 `-j` 参数选择性输出 section：

```bash
arm-none-eabi-objcopy -O ihex \
    -j .text -j .data -j .loader_param -j .loader_text -j .loader_data -j .intvec \
    rzn2l_xspi_boot_App1.elf rzn2l_xspi_boot_App1.hex
```

### 3.3 方案对比

| | 方案 A：改 .ld | 方案 B：改 objcopy |
|--|--|--|
| 改动位置 | 链接脚本 | Makefile |
| 彻底性 | 从根源解决 | 仅对当前命令有效 |
| 脆弱性 | 低 | 高，漏一个 section 就膨胀 |

**结论：方案 A 是标准做法，方案 B 是 workaround。**

### 3.4 SDK 版本差异

| SDK 版本 | .sdram_nc / .extram_nc | .heap | 烧录文件 |
|---------|----------------------|-------|---------|
| v1.10 | 无 NOLOAD | 无 NOLOAD | 膨胀 |
| v2.1 | 有 NOLOAD ✓ | 有 NOLOAD ✓ | 正常 |

**v2.1 已修复此问题**。

```ld
// v2.1 的 .ld 文件（已修复）：
.extram_nc (NOLOAD) :
{
    . = ALIGN(32);
    __extnc_start__ = .;
    KEEP(*(uncached_mem*))
    KEEP(*(nocache*))
    __extnc_end__ = .;
} > EXTRAMNC_SPACE

.heap (NOLOAD) :
{
    . = ALIGN(8);
    __heap_start__ = .;
    __HeapBase = .;
    KEEP(*(.heap))
    __HeapLimit = .;
    ...
} > HEAP_SPACE
```

## 4. 验证方法

### 4.1 readelf 查看 section 类型

```bash
arm-none-eabi-readelf -S rzn2l_xspi_boot_App1.elf
```

**修改前（SDK 默认）**：

```
  [23] .sdram_nc         PROGBITS  54500000 010000 200298 00  WA  ← PROGBITS
  [24] .heap             PROGBITS  55000000 220000 1f2020 00  WA  ← PROGBITS
  [25] .bss              NOBITS    74400100 550100 0a8944 00  WA
```

**修改后（添加 NOLOAD）**：

```
  [23] .sdram_nc         NOBITS    54500000 010000 200298 00  WA  ← NOBITS ✓
  [24] .heap             NOBITS    55000000 010000 1f2020 00  WA  ← NOBITS ✓
  [25] .bss              NOBITS    74400100 140100 0a8944 00  WA
```

### 4.2 快速筛选

```bash
arm-none-eabi-readelf -S xxx.elf | findstr "PROGBITS NOBITS"
```

- `PROGBITS` → 会写入输出文件
- `NOBITS` → 不会写入输出文件

## 5. 相关知识点

### 5.1 VMA 与 LMA

| 地址 | 全称 | 含义 |
|------|------|------|
| **VMA** | Virtual Memory Address | 运行时地址 |
| **LMA** | Load Memory Address | Flash 中的存储地址 |

**典型场景**：`.data` section
- VMA = RAM 地址 → 运行时在 RAM 中
- LMA = Flash 地址 → 初始值存在 Flash 中
- 启动代码负责从 Flash 拷贝到 RAM

**`.sdram_nc` / `.heap` 的情况**：
- VMA = LMA（相同地址）→ 无需拷贝

### 5.2 AT 关键字的不必要性

```ld
.sdram_nc SDRAM_NC_START : AT (SDRAM_NC_START) { ... } > SDRAM_NC_SPACE
```

- `SDRAM_NC_START` 决定 VMA
- `AT (SDRAM_NC_START)` 设置 LMA，但与 VMA 相同
- **LMA == VMA 时，`AT` 是冗余的**

**`AT` 的正确用途**：当 VMA ≠ LMA 时指定加载地址（如 `.data` 从 Flash 加载到 RAM）。

### 5.3 NOLOAD 的含义

`(NOLOAD)` 告诉链接器：
1. 此 section 占用内存（有 VMA），代码可以访问
2. **不需要在输出文件中分配存储空间**
3. 输出 section 类型强制设为 `SHT_NOBITS`

### 5.4 Section 分类与管理

**编译器/链接器自动管理（不用操心）**

| Section | 类型 | 原因 |
|---------|------|------|
| `.text` | PROGBITS | 代码，必须加载 |
| `.data` | PROGBITS | 初始化数据，必须加载 |
| `.rodata` | PROGBITS | 常量，必须加载 |
| `.bss` | NOBITS | 零初始化，自动不输出 |

**C 库/RTOS 内部（要小心）**

| Section | 类型 | 需要 NOLOAD |
|---------|------|------------|
| `.heap` | PROGBITS（C 库 heap 池） | **需要** |
| `.stack` | NOBITS | 不需要 |

**用户自定义（必须自己管）**

通过 `__attribute__((section("xxx")))` 定义的变量默认为 PROGBITS，**必须加 NOLOAD**。

---

## 6. 最终修改方案

在链接脚本 `fsp_xspi0_boot.ld` 中修改 `.sdram_nc` 和 `.heap` section：

```ld
.sdram_nc (NOLOAD) :
{
    . = ALIGN(32);
    __sdnc_start__ = .;
    KEEP(*(uncached_mem*))
    KEEP(*(nocache*))
    __sdnc_end__ = .;
} > SDRAM_NC_SPACE

.heap (NOLOAD) :
{
    . = ALIGN(8);
    __HeapBase = .;
    KEEP(*(.heap))
    __HeapLimit = .;
    . = ALIGN(4);
    __Heap4_start__ = .;
    */rzn/aws/amazon-freertos/freertos_kernel/portable/MemMang/heap_4.o(.bss*)
    __Heap4_end__ = .;
} > HEAP_SPACE
```

**修改要点**：
1. 添加 `(NOLOAD)` 属性
2. 去掉冗余的 `AT` 地址
3. 去掉 section 定义行中的 `SDRAM_NC_START` / `HEAP_START`（VMA 由 `>` 指定）

**修改后验证**：

```bash
arm-none-eabi-readelf -S rzn2l_xspi_boot_App1.elf | findstr ".sdram_nc .heap .bss"
```

预期输出：

```
  [xx] .sdram_nc         NOBITS    54500000 000000 201000 00  WA
  [xx] .heap             NOBITS    55000000 000000 1fffff 00  WA
  [xx] .bss              NOBITS    74400100 000000 0a8944 00  WA
```

三个 section 均为 `NOBITS`，文件偏移（Off）为 0，烧录文件大小恢复正常。
