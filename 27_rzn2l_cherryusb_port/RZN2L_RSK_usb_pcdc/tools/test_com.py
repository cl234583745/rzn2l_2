# test_com.py
import serial
print("Trying COM8...")
try:
    s = serial.Serial('COM8', 115200, timeout=1)
    print("COM8 OK")
    s.close()
except Exception as e:
    print(f"Error: {e}")