#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <stdbool.h>

#include "uart0.h"

//Check docs
#define APP_START_ADDRESS   0x0000UL
#define BL_MAGIC_BYTE       'U'
#define BL_UART_TIMEOUT     500000UL

static void jump_to_app(void)
{
    void (*app_start)(void) = (void (*)(void))APP_START_ADDRESS;

    cli();
    wdt_disable();

    uart0_deinit(); // Just release resources
    // app_start: pointer to the application start address.
    // Call directly after declaration to jump to the application program
    // (equivalent to a jump instruction to APP_START_ADDRESS and not returning to the bootloader).
    app_start();

    while (1)
    {
        // Should never return here
    }
}

static bool bootloader_should_enter(void)
{
    uint8_t cmd = 0;
    uart_status_t st = uart0_read_byte(&cmd, BL_UART_TIMEOUT);

    if (st != UART_OK)
    {
        return false;
    }

    if (cmd == BL_MAGIC_BYTE)
    {
        return true;
    }

    return false;
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
        // If UART init fails, do not stay here forever in step 1
        jump_to_app();
    }

    (void)uart0_write_line("BL alive", UART0_TIMEOUT_MAX);

    if (bootloader_should_enter())
    {
        (void)uart0_write_line("Enter Update Mode", UART0_TIMEOUT_MAX);

        while (1)
        {
            // Add later: implement a simple command protocol to receive firmware data and write to flash
        }
    }

    (void)uart0_write_line("Jumping to App...", UART0_TIMEOUT_MAX);
    jump_to_app();

    while (1)
    {
    }
}