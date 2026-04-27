
# AVR Low-Level Drivers and UART Bootloader for ATmega328P

Hands-on register-level AVR drivers and a custom UART bootloader for the ATmega328P.

## Highlights

- Low-level peripheral drivers (GPIO, UART, I2C/TWI)
- UART bootloader with page write + verify
- Python PC tool for hex streaming and update testing

## Target Hardware

- MCU: ATmega328P
- Clock: 16 MHz
- Programmer: USBasp (avrdude)
- Bootloader interface: UART

## Toolchain

- PlatformIO
- avr-gcc
- Python + pyserial (PC tool)

## Repository Structure

- Drivers in [lib/gpio_hal](lib/gpio_hal), [lib/uart_hal](lib/uart_hal), [lib/i2cMaster_hal](lib/i2cMaster_hal)
- Example apps in [src/gpio](src/gpio), [src/uart](src/uart), [src/i2cMaster](src/i2cMaster)
- Bootloader app in [src/bootloader](src/bootloader)
- Notes and docs in [docs/bootloader.md](docs/bootloader.md), [docs/datasheetNote.md](docs/datasheetNote.md), [docs/knownIssue.md](docs/knownIssue.md)

## Bootloader Features

- ISP flashing for the initial bootloader install
- Boot reset into bootloader section (fuse bits)
- UART entry via magic byte
- Command protocol with ACK/NACK
- Flash page write using SPM
- Page address validation
- 8-bit checksum validation
- Flash verification after write
- Jump to application after update

### Command Set

| Command | Description |
| ------- | ----------- |
| `U` | Enter update mode |
| `P` | Ping |
| `W` | Write one flash page |
| `J` | Jump to application |

## Memory Layout

| Region | Address Range |
| ------ | ------------- |
| Application Section | `0x0000` - `0x7BFF` |
| Bootloader Section | `0x7C00` - `0x7FFF` |

Flash page size: 128 bytes.

## Verified Workflow

1. Flash bootloader through USBasp.
2. Build application firmware.
3. Parse application `.hex` file on PC.
4. Send firmware pages over UART.
5. Bootloader writes and verifies flash pages.
6. Bootloader jumps to application.
7. Application runs successfully.

Tested app: GPIO blink on ATmega328P.

## Build and Run

Build bootloader:

```bash
pio run -e bootloader
```

Upload bootloader through ISP:

```bash
pio run -e bootloader -t upload
```

Build GPIO application:

```bash
pio run -e gpio_app
```

Send firmware through UART bootloader:

```bash
python main.py --port COM5 --baud 9600 --hex ..\.pio\build\gpio_app\firmware.hex --jump
```

## Status

Learning and experimentation project, not production-ready.

## Learning Focus

- Register-level embedded C
- AVR peripheral drivers
- Datasheet-based development
- UART protocol design
- AVR fuse bits and flash layout
- SPM-based self-programming
- Python tooling for firmware update testing