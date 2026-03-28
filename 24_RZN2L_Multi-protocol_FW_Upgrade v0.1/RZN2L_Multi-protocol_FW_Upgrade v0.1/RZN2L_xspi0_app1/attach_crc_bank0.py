#!/usr/bin/env python3
import sys
import zlib
import os

# need 1.Post-build steps:cmd: python "${ProjDirPath}\attach_crc.py"
# need 2.Output file format (-O) Raw binary 
# need 3.BUILD_DIR_NAME = "BANK0"
# need 4.BIN_BASE_NAME = "RZN2L_xspi0_app1"


# --- CONFIG: 只需修改这两个变量，使用相对路径即可 ---
# 构建输出目录名称（在项目根目录下）
BUILD_DIR_NAME = "BANK0"
# 输出二进制文件的基础名（不含 .bin）
BIN_BASE_NAME = "RZN2L_xspi0_app1_bank0"
# --- END OF CONFIG ---

def main():
    print("=== CRC Attachment Script (Auto Path) ===")

    # 核心优化：脚本自动确定自己所在目录，即为工程根目录
    # __file__ 是当前脚本文件的路径，os.path.dirname 获取其所在目录
    PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))

    # 1. 切换到工程根目录
    os.chdir(PROJECT_ROOT)
    print(f"[INFO] Project Root: {PROJECT_ROOT}")
    print(f"[INFO] Working Dir: {os.getcwd()}")

    # 2. 构建完整路径（现在基于PROJECT_ROOT）
    input_bin = os.path.join(BUILD_DIR_NAME, BIN_BASE_NAME + ".bin")
    output_bin = os.path.join(BUILD_DIR_NAME, BIN_BASE_NAME + "_with_crc.bin")

    print(f"[INFO] Input File:  {input_bin}")
    print(f"[INFO] Output File: {output_bin}")

    # 3. 检查输入文件是否存在
    if not os.path.exists(input_bin):
        print(f"[ERROR] Input file does NOT exist.")
        print(f"        Full path: {os.path.abspath(input_bin)}")
        print(f"        Please verify: BUILD_DIR_NAME='{BUILD_DIR_NAME}' and BIN_BASE_NAME='{BIN_BASE_NAME}'")
        sys.exit(1)

    # 4. 读取原始数据
    with open(input_bin, 'rb') as f:
        data = bytearray(f.read())  # 使用bytearray以便修改

    # 5. 计算最终文件大小（原始数据长度 + 4字节CRC）
    total_size = len(data) + 4
    
    # 6. 将文件大小写入第10-13字节（偏移量9-12）
    # 注意：Python字节索引从0开始，所以第10字节是索引9
    if len(data) >= 13:  # 确保文件至少有13字节
        data[9:13] = total_size.to_bytes(4, byteorder='little')
        print(f"[INFO] Set total size at offset 9-12: {total_size} bytes (0x{total_size:08X})")
    else:
        print(f"[ERROR] Input file too small ({len(data)} bytes), need at least 13 bytes for setting size at offset 9-12")
        sys.exit(1)
    
    # 7. 计算CRC-32 (标准算法)
    crc_initial = zlib.crc32(data) & 0xFFFFFFFF
    print(f"[INFO] CRC-32 (Original Value): 0x{crc_initial:08X}")

    # 8. 将CRC值以小端序追加
    crc_bytes = crc_initial.to_bytes(4, byteorder='little')

    # 9. 写入输出文件
    with open(output_bin, 'wb') as f:
        f.write(data)
        f.write(crc_bytes)

    print(f"[SUCCESS] File created: {output_bin}")
    print(f"[SUCCESS] Size: {total_size} bytes ({len(data)} original + 4 CRC)")

if __name__ == "__main__":
    main()