## Flash Memory Layout and Bootloader Programming Rule

ATmega328P divides Flash memory into two main regions:

- **Application Section**: stores the main user firmware
- **Boot Loader Section**: stores the bootloader code

Example layout for a configuration where the Boot Loader Section starts at byte address `0x7000`:

    =====================================================
     Byte Address  | Word Address | Region
    =====================================================
     0x0000        | 0x0000       |
     ...           | ...          | Application Section
     ...           | ...          | (main firmware)
     0x6FFF        | 0x37FF       |
    -----------------------------------------------------
     0x7000        | 0x3800       | Boot Loader Section
     ...           | ...          | (bootloader code)
     0x7FFF        | 0x3FFF       |
    =====================================================

A key hardware rule of the AVR bootloader architecture is:

- The `SPM` (Store Program Memory) instruction is only effective when it is executed by code running inside the **Boot Loader Section**
- If the current Program Counter (PC) is located in the **Application Section**, the `SPM` instruction is ignored by hardware

This is an important built-in protection mechanism of the AVR architecture. It prevents normal application code from accidentally modifying Flash memory, and reduces the risk of firmware corruption caused by invalid pointers, software bugs, or memory handling errors.

Because of this rule, any code responsible for updating application firmware must be placed inside the **Boot Loader Section**.