"""
test_cdc_speed.py - USB CDC 速度测试脚本 (对齐 CherryUSB)
用法: python test_cdc_speed.py COM8

DTR=0  -> 触发 MCU OUT 测试 (PC->MCU)
DTR=1  -> 触发 MCU IN 测试  (MCU->PC)
"""
import serial
import time
import sys

test_comx = sys.argv[1] if len(sys.argv) > 1 else 'COM8'
test_baudrate = 2000000
test_maxsize = 10 * 1024 * 1024  # 10MB
test_data = b'\xAA' * 4096

print(f"[USB CDC Speed Test] Target: {test_comx}")

try:
    ser = serial.Serial(test_comx, test_baudrate, timeout=1)
except Exception as e:
    print(f"Open failed: {e}")
    sys.exit(1)

def test_cdc_out():
    """PC -> MCU 测试"""
    send_count = 0
    ser.setDTR(0)       # DTR=0 触发MCU进入OUT模式
    time.sleep(0.1)

    begin = time.time()
    while send_count < 10 * 1024 * 1024:  # 发送2MB
        txlen = ser.write(test_data)
        send_count += txlen

    elapsed = time.time() - begin
    print(f"OUT: {send_count/1024/1024:.2f} MB in {elapsed:.2f}s ({send_count/1024/1024/elapsed:.2f} MB/s)")

def test_cdc_in():
    """MCU -> PC 测试"""
    read_count = 0
    ser.reset_input_buffer()
    ser.setDTR(1)       # DTR=1 触发MCU进入IN模式

    begin = time.time()
    while read_count < test_maxsize:
        data = ser.read(min(4096, test_maxsize - read_count))
        if data:
            read_count += len(data)
        else:
            break

    elapsed = time.time() - begin
    if elapsed > 0:
        print(f"IN:  {read_count/1024/1024:.2f} MB in {elapsed:.2f}s ({read_count/1024/1024/elapsed:.2f} MB/s)")

if __name__ == '__main__':
    print("--- test cdc out speed ---")
    test_cdc_out()
    time.sleep(0.5)
    print("--- test cdc in speed ---")
    test_cdc_in()
    ser.close()
    print("Done.")
