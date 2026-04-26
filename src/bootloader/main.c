#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <stdint.h>
#include <stdbool.h>

#include "uart0.h"

//Check docs
#define APP_START_ADDRESS   0x0000UL
#define BL_MAGIC_BYTE       'U'
#define BL_UART_TIMEOUT     500000UL
#define BOOT_START_ADDRESS  0x7C00UL // Bootloader start address 
static uint8_t mcusr_mirror __attribute__((section(".noinit")));

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

void wdt_init(void)
{
    mcusr_mirror = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

static void jump_to_app(void)
{   
    void (*app_start)(void);
    app_start = (void (*)(void))APP_START_ADDRESS;

    cli();
    wdt_disable();
    uart0_deinit(); 

    // Jump to app after clear everything
    app_start();

    while (1)
    {
        // Just in case app_start returns (Some guy told)
    }
}

static bool is_valid_app_page(uint32_t page_addr)
{
    if ((page_addr % SPM_PAGESIZE) != 0) // Ensure page_addr is page-aligned
    {
        return false;
    }

    if (page_addr >= BOOT_START_ADDRESS) // Must be below bootloader start address
    {
        return false;
    }

    return true;
}

static bool boot_program_page(uint32_t page_addr, const uint8_t *buf)
{
    uint16_t i;
    uint16_t w;
    uint8_t sreg;

    if (buf == NULL)
    {
        return false;
    }

    if (!is_valid_app_page(page_addr))
    {
        return false;
    }

    sreg = SREG;
    cli();

    eeprom_busy_wait();

    boot_page_erase(page_addr);
    boot_spm_busy_wait();

    for (i = 0; i < SPM_PAGESIZE; i += 2)
    {
        w = (uint16_t)buf[i] | ((uint16_t)buf[i + 1] << 8);
        boot_page_fill(page_addr + i, w);
    }

    boot_page_write(page_addr);
    boot_spm_busy_wait();

    boot_rww_enable();

    SREG = sreg;

    return true;
}

static bool bootloader_should_enter(void)
{
    uint8_t cmd = 0;
    uart_status_t st = uart0_read_byte(&cmd, BL_UART_TIMEOUT);

    if (st != UART_OK)
    {
        return false;
    }

    // Entry bootloader condition
    if (cmd == BL_MAGIC_BYTE)
    {
        return true;
    }

    return false;
}

// testing function here:
static void bootloader_process_command(void)
{
    uint8_t cmd = 0;

    if (uart0_read_byte(&cmd, UART0_TIMEOUT_MAX) != UART_OK)
    {
        return;
    }

    switch (cmd)
    {
        case 'P':
            (void)uart0_write_byte('A', UART0_TIMEOUT_MAX);
            break;

        case 'J':
            (void)uart0_write_byte('A', UART0_TIMEOUT_MAX);
            jump_to_app();
            break;

        case 'W':
            (void)uart0_write_byte('N', UART0_TIMEOUT_MAX);
            break;

        default:
            (void)uart0_write_byte('N', UART0_TIMEOUT_MAX);
            break;
    }
}

int main(void)
{
    cli();
    wdt_disable();

    uart0_config_t cfg = {
        .baud = 9600,
        .use_u2x = false,
        .databits = UART_DATABITS_8,
        .parity = UART_PARITY_NONE,
        .stopbits = UART_STOP_1
    };

    if (uart0_init(&cfg) != UART_OK)
    {
        // Incase UART init fails
        jump_to_app();
    }

    (void)uart0_write_line("BL alive", UART0_TIMEOUT_MAX);

    if (bootloader_should_enter())
    {
        (void)uart0_write_line("Enter Update Mode", UART0_TIMEOUT_MAX);

        while (1)
        {
            bootloader_process_command();
        }
    }

    (void)uart0_write_line("Jumping to App...", UART0_TIMEOUT_MAX);
    jump_to_app();

    while (1)
    {
    }
}