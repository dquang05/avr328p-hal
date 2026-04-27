import serial
import time

PORT = "COM5"
BAUD = 9600

PAGE_SIZE = 128
PAGE_ADDR = 0x7B80

data = bytes((i & 0xFF) for i in range(PAGE_SIZE))

header = (
    PAGE_ADDR.to_bytes(4, "little") +
    PAGE_SIZE.to_bytes(2, "little")
)

checksum = (ord("W") + sum(header) + sum(data)) & 0xFF
frame = b"W" + header + data + bytes([checksum])

with serial.Serial(PORT, BAUD, timeout=2) as ser:
    time.sleep(0.2)

    print("Reset MCU now...")
    time.sleep(1.5)

    print("Boot message:", ser.readline())

    ser.write(b"U")
    print("Enter message:", ser.readline())

    ser.write(b"P")
    print("Ping:", ser.read(1))

    ser.write(frame)
    print("Write:", ser.read(1))