# AVR Low-Level Drivers (ATmega328P) — GPIO / UART / SPI / I2C

A Hardware Abstraction Layer (HAL) development project for the ATmega328P microcontroller. This project focuses on implementing peripheral drivers through direct register-level manipulation, optimizing for performance and memory footprint compared to high-level frameworks.

Core Objective: To demonstrate a rigorous embedded firmware development workflow: Datasheet Analysis -> Register Mapping -> Driver Implementation -> Application Integration.

---

## Technical Features

- **GPIO**: Direct manipulation of DDRx, PORTx, and PINx registers. Includes flexible pin mapping for Arduino Uno/Nano form factors.
- **UART (USART0)**: Configurable baud rate, featuring an interrupt-driven architecture (ISR) combined with a Ring Buffer for reliable asynchronous data handling.
- **SPI Master**: Supports configurable Clock Prescalers, Data Order, and synchronous transfer modes.
- **I2C (TWI) Master**: Full implementation of Start/Stop sequences, ACK/NACK handshaking, and helper functions for interfacing with external sensor registers.
- **Development Environment**: Fully compatible with the PlatformIO ecosystem (avr-gcc) and flashed via avrdude.

---


## Build & Upload

### GPIO
```bash
pio run -e gpio
pio run -e gpio -t upload
```
Other Examples:
```bash
pio run -e uart -t upload
pio run -e spiMaster -t upload
pio run -e i2cMaster -t upload
```
---

## Design Philosophy

This project is built upon the following engineering principles:
- **Transparency**: Avoids bloated abstraction layers, ensuring the data flow remains close to the hardware metal.
- **Efficiency**: Utilizes struct-based pointers and bitwise operations to minimize CPU cycles and Flash/SRAM consumption.
- **Reliability**: Implements error handling and timeout mechanisms for hardware communication, specifically for I2C and UART bus states.

---

## Non-goals

- Not intended as a 100% drop-in replacement for the Arduino Core (Serial/Wire/SPI).
- Does not aim for multi-platform compatibility; strictly optimized for the ATmega328P.
- Avoids high-level abstractions that obscure the underlying hardware behavior.

---

## System Specifications

- **Microcontroller**: ATmega328P (8-bit AVR).
- **Clock Speed**: 16 MHz.
- **Toolchain**: PlatformIO / AVR-GCC.
- **Target Boards**: Arduino Uno, Arduino Nano.

---

## Project Structure

```text
AVRLOWLEVELREGISTER/
├── .pio/                   
├── .vscode/                
│   ├── c_cpp_properties.json
│   ├── extensions.json
│   └── launch.json
├── include/              
│   └── README
├── lib/                    
│   └── gpio/               
│       ├── driver_gpio.c
│       └── driver_gpio.h
|    └── uart/
│       ├── driver_uart.c
│       └── driver_uart.h
|    └── spiMaster/
│       ├── driver_spi.c
│       └── driver_spi.h
|    └── i2cMaster/
│       ├── driver_i2c.c
│       └── driver_i2c.h
├── src/                    
│   ├── gpio/
│   │   └── main.c
│   ├── i2cMaster/    
│   │   └── main.c      
│   ├── spiMaster/ 
│   │   └── main.c         
│   └── uart/          
│       └── main.c    
├── test/                  
├── .gitignore          
├── platformio.ini         
└── README.md             