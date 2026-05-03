#!/usr/bin/env python3
"""
test_cdc_speed.py - CherryUSB 官方 CDC 速度测试脚本

用法: python test_cdc_speed.py COM11

原理:
OUT test: Host writes 10MB to device, measures speed
IN test:  Set DTR=1, device loops sending 2048B, host reads 10MB, measures speed
"""

import serial
import time
import sys

try:
    from serial.tools.list_ports import comports
except ImportError:
    raise serial.serialutil.SerialException

test_comx = sys.argv[1] if len(sys.argv) > 1 else 'COM66'
test_baudrate = 2000000
test_maxsize = 10 * 1024 * 1024  # 10MB

test_data = b'\xAA' * 4096

test_serial = serial.Serial(test_comx, test_baudrate, timeout=1)

def test_cdc_out():
    send_count = 0
    begin = time.time()
    while True:
        if send_count < test_maxsize:
            txdatalen = test_serial.write(test_data)
            send_count += txdatalen
        else:
            elapsed = time.time() - begin
            speed_mbps = (send_count / 1024 / 1024) / elapsed
            print("cdc out speed %.3f MB/s (%d B / %.3f s)" % (speed_mbps, send_count, elapsed))
            break

def test_cdc_in():
    read_count = 0
    begin = time.time()
    while True:
        if read_count < test_maxsize:
            data = test_serial.read(test_maxsize)
            read_count += len(data)
        else:
            elapsed = time.time() - begin
            speed_mbps = (read_count / 1024 / 1024) / elapsed
            print("cdc in speed %.3f MB/s (%d B / %.3f s)" % (speed_mbps, read_count, elapsed))
            break

if __name__ == '__main__':
    print('test cdc out speed (host -> device)')
    test_serial.setDTR(0)
    test_cdc_out()

    print('test cdc in speed (device -> host)')
    test_serial.setDTR(1)
    test_cdc_in()

    test_serial.close()
