# AVR Low-Level Drivers for ATmega328P

A personal register-level embedded project for the ATmega328P, focused on writing peripheral drivers directly from the datasheet and understanding the MCU architecture at a low level.

Currently implemented drivers:
- GPIO
- UART
- I2C (TWI Master)

The project is being extended toward a custom UART bootloader for the ATmega328P, with the goal of supporting:
- initial bootloader flashing via ISP
- application update via UART
- clear memory layout
- basic checksum/CRC
- safe jump to application

## Target
- MCU: ATmega328P
- Clock: 16 MHz
- Toolchain: PlatformIO / avr-gcc
- Programmer: USBasp / avrdude

## Build & Upload

```bash
pio run -e gpio -t upload
pio run -e uart -t upload
pio run -e i2cMaster -t upload
```
## Status

This repository is mainly used for learning:

- datasheet reading
- register mapping
- low-level driver implementation
- firmware architecture design
- bootloader development