#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdbool.h>

#include "uart0.h"

/* Bootloader command bytes */
#define BL_ACK              'A'
#define BL_NACK             'N'
#define BL_CMD_P            'P'
#define BL_CMD_W            'W'
#define BL_CMD_J            'J'

/* Bootloader configuration */
#define APP_START_ADDRESS   0x0000UL
#define BOOT_START_ADDRESS  0x7C00UL
#define BL_MAGIC_BYTE       'U'
#define BL_UART_TIMEOUT     500000UL

/* Flash page configuration */
#define BL_PAGE_SIZE        ((uint16_t)SPM_PAGESIZE)
#define BL_WRITE_TIMEOUT    UART0_TIMEOUT_MAX

/* One flash page buffer. ATmega328P has 2 KB SRAM. */
static uint8_t page_buf[SPM_PAGESIZE];

/*
 * Disable watchdog early after reset.
 * This runs before main() from the .init3 section.
 */
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

void wdt_init(void)
{
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

    app_start();

    while (1)
    {
        /* Should never return here. */
    }
}

static bool is_valid_app_page(uint32_t page_addr)
{
    if ((page_addr % SPM_PAGESIZE) != 0)
    {
        return false;
    }

    if (page_addr > (BOOT_START_ADDRESS - SPM_PAGESIZE))
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

static bool boot_verify_page(uint32_t page_addr, const uint8_t *buf)
{
    if (buf == NULL)
    {
        return false;
    }

    if (!is_valid_app_page(page_addr))
    {
        return false;
    }

    for (uint16_t i = 0; i < SPM_PAGESIZE; i++)
    {
        uint8_t flash_byte;

        flash_byte = pgm_read_byte((const void *)(uintptr_t)(page_addr + i));

        if (flash_byte != buf[i])
        {
            return false;
        }
    }

    return true;
}

static bool bootloader_should_enter(void)
{
    uint8_t cmd = 0;
    uart_status_t st;

    st = uart0_read_byte(&cmd, BL_UART_TIMEOUT);

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

static bool uart_read_bytes(uint8_t *dst, uint16_t len, uint8_t *sum)
{
    uint8_t b;

    if (dst == NULL)
    {
        return false;
    }

    for (uint16_t i = 0; i < len; i++)
    {
        if (uart0_read_byte(&b, BL_WRITE_TIMEOUT) != UART_OK)
        {
            return false;
        }

        dst[i] = b;

        if (sum != NULL)
        {
            *sum = (uint8_t)(*sum + b);
        }
    }

    return true;
}

static uint16_t read_u16_le(const uint8_t *p)
{
    return ((uint16_t)p[0]) |
           ((uint16_t)p[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *p)
{
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static bool bootloader_handle_write_page(void)
{
    uint8_t header[6];
    uint8_t calc_checksum = (uint8_t)BL_CMD_W;
    uint8_t recv_checksum = 0;

    uint32_t page_addr;
    uint16_t len;

    if (!uart_read_bytes(header, sizeof(header), &calc_checksum))
    {
        return false;
    }

    if (!uart_read_bytes(page_buf, BL_PAGE_SIZE, &calc_checksum))
    {
        return false;
    }

    if (uart0_read_byte(&recv_checksum, BL_WRITE_TIMEOUT) != UART_OK)
    {
        return false;
    }

    page_addr = read_u32_le(&header[0]);
    len = read_u16_le(&header[4]);

    if (recv_checksum != calc_checksum)
    {
        return false;
    }

    if (len != BL_PAGE_SIZE)
    {
        return false;
    }

    if (!is_valid_app_page(page_addr))
    {
        return false;
    }

    if (!boot_program_page(page_addr, page_buf))
    {
        return false;
    }

    if (!boot_verify_page(page_addr, page_buf))
    {
        return false;
    }

    return true;
}

static void bootloader_process_command(void)
{
    uint8_t cmd = 0;

    if (uart0_read_byte(&cmd, UART0_TIMEOUT_MAX) != UART_OK)
    {
        return;
    }

    switch (cmd)
    {
        case BL_CMD_P:
            (void)uart0_write_byte(BL_ACK, UART0_TIMEOUT_MAX);
            break;

        case BL_CMD_J:
            (void)uart0_write_byte(BL_ACK, UART0_TIMEOUT_MAX);
            jump_to_app();
            break;

        case BL_CMD_W:
            if (bootloader_handle_write_page())
            {
                (void)uart0_write_byte(BL_ACK, UART0_TIMEOUT_MAX);
            }
            else
            {
                (void)uart0_write_byte(BL_NACK, UART0_TIMEOUT_MAX);
            }
            break;

        default:
            (void)uart0_write_byte(BL_NACK, UART0_TIMEOUT_MAX);
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