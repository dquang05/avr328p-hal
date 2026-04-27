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

with serial.Serial(PORT, BAUD, timeout=0.2) as ser:
    time.sleep(0.2)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print("Reset MCU now, then press Enter immediately...")
    input()

    # Send magic byte immediately after reset.
    ser.write(b"U")

    time.sleep(0.2)
    data_in = ser.read(128)
    print("Enter data:", data_in)

    ser.reset_input_buffer()

    ser.write(b"P")
    print("Ping:", ser.read(1))

    ser.write(frame)
    print("Write:", ser.read(1))