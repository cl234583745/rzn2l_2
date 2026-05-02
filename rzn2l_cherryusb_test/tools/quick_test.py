"""快速测试 TX 吞吐量"""
import serial, time
ser = serial.Serial('COM11', timeout=10)
ser.dtr = True; ser.rts = True
time.sleep(2)
ser.write(b'#TX 65536\r\n')
data = ser.read(65536)
print(f'Received: {len(data)} bytes')
if len(data) >= 4:
    print(f'First 4: {data[:4].hex()} (expect 00010203)')
ser.close()
