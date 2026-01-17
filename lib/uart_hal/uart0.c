#include "uart0.h"
#include <avr/io.h>

#ifndef UART0_TIMEOUT_MAX
#define UART0_TIMEOUT_MAX 0xFFFFFFFFUL
#endif

// ----------------- Small helpers -----------------
static inline bool uart0_tx_ready(void)
{
    return (UCSR0A & (1 << UDRE0)) != 0;
}

static inline bool uart0_rx_ready(void)
{
    return (UCSR0A & (1 << RXC0)) != 0;
}

static inline uart_status_t uart0_wait_flag(volatile uint8_t *reg, uint8_t mask, bool set, uint32_t timeout)
{
    while (timeout--)
    {
        if (set)
        {
            if (*reg & mask)
                return UART_OK; // bit set
        }
        else
        {
            if (!(*reg & mask))
                return UART_OK; // bit clear
        }
    }

    return UART_ERR_TIMEOUT;
}

static inline uint16_t uart0_calc_ubrr(uint32_t baud, bool u2x)
{
    if (baud == 0)
        return 0;

    uint32_t div = u2x ? 8UL : 16UL;
    uint32_t ubrr = (F_CPU / (div * baud)) - 1UL;

    if (ubrr > 0x0FFFUL)
        ubrr = 0x0FFFUL; // 12-bit UBRR limit
    return (uint16_t)ubrr;
}

// ----------------- Public API -----------------
uart_status_t uart0_init(const uart0_config_t *cfg)
{
    if (!cfg || cfg->baud == 0)
        return UART_ERR_PARAM;

    // clear RXEN0, TXEN0 before configuring
    UCSR0B &= ~((1 << TXEN0) | (1 << RXEN0));
    // Set speed mode U2X0
    if (cfg->use_u2x)
    {
        UCSR0A |= (1 << U2X0);
    }
    else
    {
        UCSR0A &= ~(1 << U2X0);
    }
    // 3) Set baud (UBRR0H/UBRR0L)
    uint16_t ubrr = uart0_calc_ubrr(cfg->baud, cfg->use_u2x);
    UBRR0H = (uint8_t)((ubrr >> 8) & 0x0F);
    UBRR0L = (uint8_t)(ubrr & 0xFF);

    // Frame format: databits/parity/stopbits => UCSR0C (and UCSZ02 in UCSR0B)
    // Clear UCSZ02
    UCSR0B &= ~(1 << UCSZ02);
    // Set UCSZ01:0
    UCSR0C &= ~((1 << UCSZ01) | (1 << UCSZ00)); // Clear first
    switch (cfg->databits)
    {
    case UART_DATABITS_5:
        // 5 bits: UCSZ02=0, UCSZ01=0, UCSZ00=0
        break;
    case UART_DATABITS_6:
        UCSR0C |= (1 << UCSZ00); // UCSZ01=0, UCSZ00=1
        break;
    case UART_DATABITS_7:
        UCSR0C |= (1 << UCSZ01); // UCSZ01=1, UCSZ00=0
        break;
    case UART_DATABITS_8:
        UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); // UCSZ01=1, UCSZ00=1
        break;
    default:
        return UART_ERR_PARAM;
    }
    // set UPM01:0 (parity), USBS0 (stop)
    // stopbits
    if (cfg->stopbits == UART_STOP_2)
        UCSR0C |= (1 << USBS0);
    else
        UCSR0C &= ~(1 << USBS0);

    // parity
    UCSR0C &= ~((1 << UPM01) | (1 << UPM00));
    if (cfg->parity == UART_PARITY_EVEN)
        UCSR0C |= (1 << UPM01);
    else if (cfg->parity == UART_PARITY_ODD)
        UCSR0C |= (1 << UPM01) | (1 << UPM00);
    // Enable RX/TX
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);

    // (Optional) Flush UDR0
    (void)UDR0;
    return UART_OK;
}

void uart0_deinit(void)
{
    // disable RX/TX, optionally disable interrupts
    UCSR0B &= ~((1 << TXEN0) | (1 << RXEN0));

}

// ----------------- TX -----------------
uart_status_t uart0_write_byte(uint8_t b, uint32_t timeout)
{
 
    uart_status_t st = uart0_wait_flag(&UCSR0A, (1 << UDRE0), true, timeout);
    if (st != UART_OK)
        return st;
    UDR0 = b;
    return UART_OK;
}

uart_status_t uart0_write(const uint8_t *buf, size_t len, uint32_t timeout)
{
    if (!buf && len)
        return UART_ERR_PARAM;

    for (size_t i = 0; i < len; i++)
    {
        uart_status_t st = uart0_write_byte(buf[i], timeout);
        if (st != UART_OK)
            return st;
    }
    return UART_OK;
}

uart_status_t uart0_write_str(const char *s, uint32_t timeout)
{
    if (!s)
        return UART_ERR_PARAM;
    while (*s)
    {
        uart_status_t st = uart0_write_byte((uint8_t)*s++, timeout);
        if (st != UART_OK)
            return st;
    }
    return UART_OK;
}

uart_status_t uart0_write_line(const char *s, uint32_t timeout)
{
    // write_str(s) + "\r\n"
    uart_status_t st = uart0_write_str(s, timeout);
    if (st != UART_OK)
        return st;
    st = uart0_write_str("\r\n", timeout);
    if (st != UART_OK)
        return st;
    return UART_OK;
}

// ----------------- RX -----------------
// ----------------- RX -----------------
uart_status_t uart0_read_byte(uint8_t *out, uint32_t timeout)
{
    if (!out) return UART_ERR_PARAM;

    // Wait for data available (RXC0 = 1)
    uart_status_t st = uart0_wait_flag(&UCSR0A, (1 << RXC0), true, timeout);
    if (st != UART_OK) return st;

    // Check errors BEFORE reading UDR0
    // Datasheet note: FE0/DOR0/UPE0 are valid for the received frame in UDR0
    uint8_t status = UCSR0A;

    if (status & ((1 << FE0) | (1 << DOR0) | (1 << UPE0))) {
        // Read UDR0 anyway to clear RXC0 and flush the bad byte
        (void)UDR0;

        return UART_ERR_HW;
    }

    // 4) Read data (this clears RXC0)
    *out = UDR0;
    return UART_OK;
}


uart_status_t uart0_read(uint8_t *buf, size_t len, uint32_t timeout)
{
    if (!buf && len)
        return UART_ERR_PARAM;

    for (size_t i = 0; i < len; i++)
    {
        uart_status_t st = uart0_read_byte(&buf[i], timeout);
        if (st != UART_OK)
            return st;
    }
    return UART_OK;
}
