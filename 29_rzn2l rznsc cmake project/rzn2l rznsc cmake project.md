
二十九、RZN2L rznsc生成CMake工程启动地址异常问题分析
===
[toc]

# 一、问题现象

使用瑞萨代码生成工具rznsc生成RZN2L的CMake工程，在VSCode中编译、下载到板子后，代码没有正常运行。

| 编译方式 | 工程来源 | 运行状态 |
|----------|----------|----------|
| e2studio内部构建 | e2studio工程 | ✅ 正常运行 |
| VSCode + CMake + GCC | rznsc生成的CMake工程 | ❌ 不运行 |

**关键现象**：同一份FSP配置、同一份源码，仅构建系统不同，烧录后行为完全不同。

# 二、验证方法

## 2.1 对比map文件

分别用e2studio和CMake工程编译，对比生成的map文件：

```bash
# e2studio构建产物
rzn2l_e2studio.map

# CMake构建产物
build_gcc/rzn2l_cmake.map
```

发现CMake工程map文件中`.loader_text`、`.intvec`、`.text`等关键段的加载地址与e2studio产物不一致，启动地址异常，导致上电后CPU取不到正确的loader和复位向量。

## 2.2 对比编译选项与启动代码

进一步对比两边的编译/链接选项和启动文件链接方式，发现CMake工程默认生成的`cmake/gcc.cmake`和`script/fsp_xspi0_boot.ld`存在3处与e2studio不一致的关键差异。

# 三、根本原因分析

rznsc生成的CMake工程是通用模板，没有针对RZN2L的xSPI0 boot启动流程做完整适配，需要修改3处才能与e2studio产物对齐。

## 3.1 工具链路径未配置

`Config.cmake`中`CMAKE_FIND_ROOT_PATH`默认是注释状态，未指向本地GNU Arm工具链：

```cmake
# Set the path to your GNU Arm toolchain's bin folder
set(CMAKE_FIND_ROOT_PATH "D:/Program Files/GCC/arm-gnu-toolchain-13.3.rel1-mingw-w64-i686-arm-none-eabi/bin")
```

不配置则`cmake/gcc.cmake`会直接`FATAL_ERROR`，无法编译。

## 3.2 编译/链接选项与启动代码缺失

rznsc生成的`cmake/gcc.cmake`默认只设置了编译器路径，缺少RZN2L启动所需的关键链接选项：

| 选项 | 作用 |
|------|------|
| `-mfloat-abi=hard -mfpu=neon-fp-armv8` | Cortex-R52硬浮点/NEON |
| `-nostartfiles` | 不使用标准启动文件，使用FSP的`system_init` |
| `-Wl,-e,system_init` | 指定入口符号为`system_init` |
| `--specs=nosys.specs` | 无系统调用stub |
| `-u _printf_float` | 启用浮点printf |

缺少`-Wl,-e,system_init`和`-nostartfiles`时，链接器使用默认入口，启动地址错乱，与map文件中观察到的异常一致。

## 3.3 链接脚本中`.o`通配不匹配CMake产物

`script/fsp_xspi0_boot.ld`中loader段使用`*.o(.text*)`匹配启动相关object：

```ld
*/fsp/src/bsp/cmsis/Device/RENESAS/Source/*.o(.text*)
*/fsp/src/bsp/mcu/all/*/bsp_irq_core.o(.text*)
```

e2studio产物object后缀是`.o`，而CMake + MinGW Makefiles产物后缀是`.c.obj`，导致loader段通配匹配不到任何object，启动代码没有被放到`LOADER_TEXT_ADDRESS(0x00102000)`，启动地址异常。

# 四、解决方案

针对上述3处问题，需要在rznsc生成的新工程上手动修改：

## 4.1 修改`Config.cmake`配置GCC路径

```cmake
# Set the path to your GNU Arm toolchain's bin folder
set(CMAKE_FIND_ROOT_PATH "D:/Program Files/GCC/arm-gnu-toolchain-13.3.rel1-mingw-w64-i686-arm-none-eabi/bin")
```

## 4.2 修改`cmake/gcc.cmake`编译选项与启动代码

在`gcc.cmake`末尾追加override块，强制使用`system_init`入口和FSP启动流程：

```cmake
# BEGIN RZN2L CMAKE TOOLCHAIN OVERRIDES
SET(CMAKE_CXX_FLAGS "${RASC_CMAKE_CXX_FLAGS} -Og -mfloat-abi=hard -mfpu=neon-fp-armv8")
SET(CMAKE_ASM_FLAGS "${RASC_CMAKE_ASM_FLAGS} -mfloat-abi=hard -mfpu=neon-fp-armv8")
SET(CMAKE_C_FLAGS "${RASC_CMAKE_C_FLAGS} -Og -mfloat-abi=hard -mfpu=neon-fp-armv8")
SET(CMAKE_EXE_LINKER_FLAGS "${RASC_CMAKE_EXE_LINKER_FLAGS} -Og -mfloat-abi=hard -mfpu=neon-fp-armv8 -nostartfiles -u _printf_float --specs=nosys.specs -Wl,-e,system_init")
# END RZN2L CMAKE TOOLCHAIN OVERRIDES
```

## 4.3 修改`script/fsp_xspi0_boot.ld`中`.o`为`.obj`

在loader段每条`*.o(...)`下面追加对应的`*.c.obj(...)`匹配，兼容CMake产物后缀：

```ld
*/fsp/src/bsp/cmsis/Device/RENESAS/Source/*.o(.text*)
*/fsp/src/bsp/cmsis/Device/RENESAS/Source/*.c.obj(.text*)
*/fsp/src/bsp/mcu/all/*/bsp_irq_core.o(.text*)
*/fsp/src/bsp/mcu/all/*/bsp_irq_core.c.obj(.text*)
...
```

# 五、自动化修复脚本

为了以后新建的rznsc CMake工程都能快速修复、不再踩坑，将上述3处修改以及烧录步骤做成脚本，统一放在`cmake_tools/`文件夹中。

## 5.1 目录结构

```
cmake_tools/
├── README.txt          # 使用说明
├── setup_cmake.bat     # 一键修复入口（调用ps1）
├── setup_cmake.ps1     # 自动修复3处问题的核心脚本
├── build.bat           # 增量编译
├── rebuild.bat         # 清理后全量编译
├── flash.bat           # 调用J-Link烧录srec
└── flash.jlink         # J-Link命令脚本模板
```

## 5.2 `setup_cmake.ps1` 自动修复逻辑

脚本按顺序完成3处修复：

| 步骤 | 修改文件 | 修改内容 |
|------|----------|----------|
| 1 | `Config.cmake` | 校验并写回`CMAKE_FIND_ROOT_PATH`，找不到时回退默认路径 |
| 2 | `cmake/gcc.cmake` | 幂等追加`BEGIN/END RZN2L CMAKE TOOLCHAIN OVERRIDES`覆盖块 |
| 3 | `script/fsp_xspi0_boot.ld` | 在`.loader_text`/`.loader_data`段中为每条`*.o(`追加`*.c.obj(`匹配 |

脚本幂等，重复执行不会重复追加；执行前会校验`mingw32-make.exe`是否在PATH中。

## 5.3 编译脚本

`build.bat`（增量）：

```bash
# 首次执行会自动初始化build_gcc目录并调用setup_cmake.bat
.\cmake_tools\build.bat
```

`rebuild.bat`（全量，先删除`build_gcc`）：

```bash
.\cmake_tools\rebuild.bat
```

两者内部都会先调用`setup_cmake.bat`确保3处修复已生效，再用`MinGW Makefiles`生成器配置CMake：

```bash
cmake -S . -B build_gcc -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake
cmake --build build_gcc
```

## 5.4 烧录脚本

`flash.bat`使用SEGGER J-Link Commander下载`build_gcc/rzn2l_cmake.srec`到板子：

```bash
# 自动从CMakeLists.txt解析项目名定位srec
.\cmake_tools\flash.bat

# 也可显式指定srec路径
.\cmake_tools\flash.bat D:\path\to\firmware.srec
```

`flash.jlink`为命令模板，`flash.bat`运行时将`__SREC__`占位符替换为实际路径，再调用：

```bash
JLink.exe -device R9A07G084M04 -if swd -speed 4000 -CommanderScript "%TEMP%\flash.jlink"
```

## 5.5 使用流程

```bash
# 1、首次或修改FSP配置后执行全量编译（自动修复3处问题）
.\cmake_tools\rebuild.bat

# 2、日常增量编译
.\cmake_tools\build.bat

# 3、烧录到板子
.\cmake_tools\flash.bat
```

详细说明可参考`cmake_tools/README.txt`。

# 六、验证修复

## 6.1 重新编译

```bash
.\cmake_tools\rebuild.bat
```

## 6.2 对比map文件

对比`build_gcc/rzn2l_cmake.map`与e2studio产物，`.loader_text`应位于`0x00102000`，`.intvec`位于`0x00000000`，`.text`位于`0x00000100`，与e2studio一致。

## 6.3 烧录运行

```bash
.\cmake_tools\flash.bat
```

板子复位后程序正常进入`system_init`并运行到`hal_entry`。

# 七、总结

| 项目 | 说明 |
|------|------|
| 问题类型 | rznsc生成的CMake工程模板未完整适配RZN2L启动流程 |
| 根本原因 | 工具链路径未配置 + 链接选项缺少`system_init`入口 + ld脚本`.o`通配不匹配CMake产物后缀 |
| 影响范围 | 所有rznsc生成的RZN2L CMake工程 |
| 推荐方案 | 使用`cmake_tools/setup_cmake.bat`一键修复，再`build.bat`/`flash.bat` |
| 验证方法 | 对比map文件启动段地址 + 烧录后板子正常运行 |

**核心要点**：

        1、rznsc生成的CMake工程需修改3处才能正常运行：Config.cmake工具链路径、gcc.cmake链接选项与system_init入口、fsp_xspi0_boot.ld中.o补充.c.obj匹配
        2、CMake + MinGW Makefiles产物object后缀是.c.obj而非.o，是ld脚本通配失效的根因
        3、cmake_tools脚本幂等可重复执行，新建工程直接复制该文件夹即可一键修复并完成编译烧录