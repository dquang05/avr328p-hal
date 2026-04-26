# ATmega328P Bootloader Notes

## 1. Bootloader goal
A bootloader is a small firmware stored in the Boot Loader Section.
Its job is to decide whether to stay in update mode or jump to the main application.

## 2. Current bootloader flow
- MCU resets
- Bootloader starts first (requires proper fuse settings)
- UART is initialized
- Bootloader sends "BL alive"
- It waits for a magic byte 'U' within a timeout
- If 'U' is received, it enters update mode
- Otherwise, it jumps to the application at 0x0000

## 3. Why timeout is needed
The bootloader should not block startup forever.
If no update request is received, control should be handed to the main application quickly.

## 4. Why jump_to_app() is needed
The bootloader is not the main firmware.
Its role is temporary: either update firmware or transfer execution to the application.

## 5. Cleanup before jumping
Before jumping to the application:
- disable interrupts
- disable watchdog
- deinitialize UART if needed
This helps reduce unexpected behavior in the application.

## Fuse bits related to bootloader

### 1. BOOTRST
BOOTRST decides where the MCU starts execution after reset.

- BOOTRST programmed (bit = 0): reset starts from Boot Loader Section
- BOOTRST unprogrammed (bit = 1): reset starts from Application Section

For a UART bootloader, BOOTRST should be programmed so the bootloader runs first.

### 2. BOOTSZ1:0
BOOTSZ1:0 select the size of the Boot Loader Section.

This also determines the start address of the bootloader region in Flash.
The bootloader code must fit completely inside this region.

### 3. Important AVR fuse rule
Many AVR fuses are active-low:
- bit = 0 means programmed / enabled
- bit = 1 means unprogrammed / disabled

This is easy to misunderstand when checking fuse values.

### 4. Clock-related fuse impact
UART depends on correct clock configuration.
If the fuse-selected clock source or divider does not match the firmware assumption
(e.g. F_CPU = 16 MHz), baud rate may be wrong and serial communication may fail.

### 5. Practical implication for this project
To make the bootloader work correctly:
- bootloader code must be placed in Boot Loader Section
- BOOTRST must direct reset to boot section
- BOOTSZ must match the chosen bootloader size
- clock fuse must match the UART baud calculation

## Bootloader flash write notes

### 1. Flash addressing model
Program Flash is a single memory region, but it can be addressed in two ways:
- word address (1 word = 2 bytes)
- byte address

### 2. boot_page_fill(addr, data) meaning
For boot_page_fill(addr, data):
- addr is a byte address
- data is one word (16-bit)
- the lower byte of data (LSB) goes to the lower byte address
- the upper byte of data (MSB) goes to the higher byte address

### 3. Byte-to-word packing example
If the input buffer contains two bytes [0x11, 0x22], the word must be packed as 0x2211.

### 4. Page-based Flash programming
Flash is not programmed one standalone byte at a time.
Programming is page-based.

### 5. Minimal write sequence
A minimal bootloader write flow is:
- erase page
- fill temporary page buffer word by word
- write page
- call boot_rww_enable

### 6. Boot section requirement
SPM (self-programming) instructions are only effective when code is running from the Boot Loader Section.

