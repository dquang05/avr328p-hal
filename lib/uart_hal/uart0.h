#ifndef UART0_H
#define UART0_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

typedef enum {
    UART_OK = 0,
    UART_ERR_PARAM,
    UART_ERR_TIMEOUT,
    UART_ERR_HW
} uart_status_t;

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
} uart_parity_t;

typedef enum {
    UART_STOP_1 = 0,
    UART_STOP_2
} uart_stopbits_t;

typedef enum {
    UART_DATABITS_5 = 5,
    UART_DATABITS_6 = 6,
    UART_DATABITS_7 = 7,
    UART_DATABITS_8 = 8
    // optionally add 9 bits later
} uart_databits_t;

typedef struct {
    uint32_t baud;
    uart_databits_t databits;     // default 8
    uart_parity_t parity;         // default none
    uart_stopbits_t stopbits;     // default 1
    bool use_u2x;                 // true: U2X0=1 (most common baud rates will be more accurates)
} uart0_config_t;

// ---------- Core ----------
uart_status_t uart0_init(const uart0_config_t *cfg);
void          uart0_deinit(void);

// ---------- TX (blocking/polling) ----------
uart_status_t uart0_write_byte(uint8_t b, uint32_t timeout);
uart_status_t uart0_write(const uint8_t *buf, size_t len, uint32_t timeout);

// Helpers tiện cho debug/portfolio
uart_status_t uart0_write_str(const char *s, uint32_t timeout);
uart_status_t uart0_write_line(const char *s, uint32_t timeout); // append "\r\n"

// ---------- RX (blocking/polling) ----------
uart_status_t uart0_read_byte(uint8_t *out, uint32_t timeout);
uart_status_t uart0_read(uint8_t *buf, size_t len, uint32_t timeout);

// ---------- Status helpers ----------
static inline bool uart0_tx_ready(void); // UDRE0
static inline bool uart0_rx_ready(void); // RXC0

#endif
