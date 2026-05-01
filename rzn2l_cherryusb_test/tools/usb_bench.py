"""
usb_bench.py - USB CDC ACM 吞吐量测试脚本 (CherryUSB vs FSP对比)

用法:
  1. 安装依赖: pip install pyserial
  2. 修改 PORT 为设备串口号 (如 'COM5')
  3. 运行: python usb_bench.py

设备端命令:
  #TX N     → 设备发送N字节测试数据 (N≤1MB)
  #ECHO     → 普通回环模式
"""

import serial
import time
import struct

PORT = 'COM10'           # 改为你的设备串口号
# Windows: COM>=10 需要 \\\\.\\ 前缀，否则打不开
for prefix in ('COM', ):
    if PORT.startswith(prefix):
        try:
            n = int(PORT[len(prefix):])
            if n >= 10:
                PORT = '\\\\.\\' + PORT
        except: pass
BAUD = 115200           # 调试串口波特率, 不影响USB
CHUNK = 64              # USB bulk 包大小
VERIFY = True           # 校验接收数据

def bench_tx(ser, total_bytes):
    """测试: PC→设备(发命令), 设备→PC(发N字节), 测量吞吐量"""
    cmd = f'#TX {total_bytes}\r\n'.encode()
    ser.write(cmd)

    start = time.time()
    received = 0
    while received < total_bytes:
        chunk = ser.read(min(CHUNK, total_bytes - received))
        if VERIFY:
            for i, b in enumerate(chunk):
                expected = (received + i) & 0xFF
                if b != expected:
                    print(f'DATA ERR at offset {received+i}: got {b:02X} expected {expected:02X}')
                    return
        received += len(chunk)
    elapsed = time.time() - start

    kbps = received / elapsed / 1024
    print(f'TX bench: {received} B / {elapsed*1000:.0f} ms = {kbps:.1f} KB/s')

def bench_echo(ser, total_bytes):
    """测试: PC→设备(发数据), 设备→PC(echo), 测量往返吞吐量"""
    data = bytes([i & 0xFF for i in range(CHUNK)])
    loops = total_bytes // CHUNK

    start = time.time()
    for _ in range(loops):
        ser.write(data)
        rx = ser.read(CHUNK)
        if rx != data:
            print('ECHO MISMATCH!')
            return
    elapsed = time.time() - start

    kbps = (loops * CHUNK) / elapsed / 1024
    print(f'ECHO bench: {loops * CHUNK} B / {elapsed*1000:.0f} ms = {kbps:.1f} KB/s (round-trip)')

def main():
    print(f'Opening {PORT}...')
    ser = serial.Serial(PORT, baudrate=115200, timeout=5)
    ser.dtr = True
    ser.rts = True
    time.sleep(2)  # 等Windows CDC驱动初始化

    print('=== USB CDC ACM Benchmark ===')
    print()

    # 测试1: 设备TX吞吐量
    for n in [1024, 4096, 16384, 65536, 131072]:
        bench_tx(ser, n)

    print()

    # 测试2: Echo往返吞吐量
    for n in [1024, 4096, 16384, 65536]:
        bench_echo(ser, n)

    ser.close()

if __name__ == '__main__':
    main()
