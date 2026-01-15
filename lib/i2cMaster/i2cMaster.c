// 1) Status-driven state checks:
//      - After every TWI action (START, SLA+R/W, DATA, RX), wait for TWINT and then verify TWSR.
//      - Each step maps the hardware status into a small set of return codes:
//          I2C_OK, I2C_NACK, I2C_TIMEOUT_ERR, I2C_ERROR.
//
//   2) Timeout protection:
//      - twi_wait_twint() polls TWINT with a configurable I2C_TIMEOUT counter to avoid deadlocks.
//
//   3) START vs REPEATED START:
//      - START and REPEATED START are generated identically (set TWSTA).
//      - twi_send_start(allow_repeated) accepts either TW_START or (optionally) TW_REP_START.
//      - Separate public wrappers (i2c_start_* / i2c_restart_*) make call-sites explicit.
//
//   4) Address phase abstraction:
//      - twi_send_sla() transmits SLA+R/W and validates the expected ACK/NACK codes for either
//        Master Transmitter or Master Receiver mode.
//
//   5) Byte-level IO:
//      - i2c_write(): send one data byte and check ACK/NACK.
//      - i2c_read_ack(): receive a byte and return ACK (TWEA=1) to continue reading.
//      - i2c_read_nack(): receive the last byte and return NACK (TWEA=0) to end reading.
//
//   6) STOP handling:
//      - i2c_stop() issues TWSTO and (optionally) waits until the STOP condition is released,
//        ensuring the bus becomes idle before returning.
//
//   7) High-level helpers:
//      - i2c_write_bytes() / i2c_read_bytes(): burst transfers with proper final NACK on read.
//      - i2c_write_reg(): START(W) + reg + data... + STOP (common "register write" pattern).
//      - i2c_read_reg():  START(W) + reg + RESTART(R) + data... + STOP (common "register read" pattern).

#include "i2cMaster.h"
#include <avr/io.h>

// --------- TWI status codes (ATmega328P datasheet) ----------
#define TW_STATUS_MASK 0xF8

// Start / Repeated start
#define TW_START 0x08
#define TW_REP_START 0x10

// Master Transmitter
#define TW_MT_SLA_ACK 0x18
#define TW_MT_SLA_NACK 0x20
#define TW_MT_DATA_ACK 0x28
#define TW_MT_DATA_NACK 0x30
#define TW_MT_ARB_LOST 0x38

// Master Receiver
#define TW_MR_SLA_ACK 0x40
#define TW_MR_SLA_NACK 0x48
#define TW_MR_DATA_ACK 0x50
#define TW_MR_DATA_NACK 0x58

// Misc
#define TW_BUS_ERROR 0x00

// Read twi status
static inline uint8_t twi_status(void)
{
    // 0xF8 = 1111 1000, this masks the last 3 bits - prescaler bits,
    // read only first 5 bits - status bits(just saw it somewhere in the datasheet)
    return (uint8_t)(TWSR & TW_STATUS_MASK);
}
// Polling TWINT flag with timeout (it set to 1 when operation complete)
static i2c_status_t twi_wait_twint(void)
{
    uint32_t t = I2C_TIMEOUT;
    while (!(TWCR & (1 << TWINT)))
    {
        if (--t == 0)
            return I2C_TIMEOUT_ERR;
    }
    return I2C_OK;
}

static i2c_status_t twi_send_start(uint8_t allow_repeated)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    i2c_status_t st = twi_wait_twint(); // Wait for TWINT flag set
    if (st != I2C_OK)
        return st;

    uint8_t s = twi_status();
    if (s == TW_START)
        return I2C_OK;
    if (allow_repeated && s == TW_REP_START)
        return I2C_OK;

    if (s == TW_MT_ARB_LOST)
        return I2C_ERROR;
    if (s == TW_BUS_ERROR)
        return I2C_ERROR;
    return I2C_ERROR;
}

static i2c_status_t twi_send_sla(uint8_t sla_rw, uint8_t expect_ack, uint8_t expect_nack)
{
    TWDR = sla_rw;
    TWCR = (1 << TWINT) | (1 << TWEN);
    i2c_status_t st = twi_wait_twint();
    if (st != I2C_OK)
        return st;

    uint8_t s = twi_status();
    if (s == expect_ack)
        return I2C_OK;
    if (s == expect_nack)
        return I2C_NACK;

    if (s == TW_MT_ARB_LOST)
        return I2C_ERROR;
    if (s == TW_BUS_ERROR)
        return I2C_ERROR;
    return I2C_ERROR;
}

void i2c_init(void)
{
    // prescaler = 1 => TWPS = 0
    TWSR = 0x00;

    // fSCL = F_CPU / (16 + 2*TWBR*prescaler)
    // => TWBR = ((F_CPU/fSCL) - 16) / 2
    uint32_t twbr = ((F_CPU / I2C_SCL_FREQ) - 16UL) / 2UL;
    if (twbr > 255UL)
        twbr = 255UL; // 8-bit register limit
    TWBR = (uint8_t)twbr;

    // Enable TWI
    TWCR = (1 << TWEN);
};

//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------

i2c_status_t i2c_start_write(uint8_t addr7)
{
    i2c_status_t st = twi_send_start(0); // Create START condition
    if (st != I2C_OK)
        return st;
    return twi_send_sla((uint8_t)((addr7 << 1) | 0), TW_MT_SLA_ACK, TW_MT_SLA_NACK);
}

i2c_status_t i2c_start_read(uint8_t addr7)
{
    i2c_status_t st = twi_send_start(0);
    if (st != I2C_OK)
        return st;
    return twi_send_sla((uint8_t)((addr7 << 1) | 1), TW_MR_SLA_ACK, TW_MR_SLA_NACK);
}

i2c_status_t i2c_restart_write(uint8_t addr7)
{
    i2c_status_t st = twi_send_start(1);
    if (st != I2C_OK)
        return st;
    return twi_send_sla((uint8_t)((addr7 << 1) | 0), TW_MT_SLA_ACK, TW_MT_SLA_NACK);
}

i2c_status_t i2c_restart_read(uint8_t addr7)
{
    i2c_status_t st = twi_send_start(1);
    if (st != I2C_OK)
        return st;
    return twi_send_sla((uint8_t)((addr7 << 1) | 1), TW_MR_SLA_ACK, TW_MR_SLA_NACK);
}

void i2c_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    // Datasheet: TWINT is NOT set after a STOP condition has been sent (so wait for TWSTO here)
    uint32_t t = I2C_TIMEOUT;
    while ((TWCR & (1 << TWSTO)) && --t)
    {
        ;
    }
}

i2c_status_t i2c_write(uint8_t data);

i2c_status_t i2c_read_ack(uint8_t *out);
i2c_status_t i2c_read_nack(uint8_t *out);

// Helpers (khuyến nghị cho app layer)
i2c_status_t i2c_write_bytes(uint8_t addr7, const uint8_t *data, uint16_t len);
i2c_status_t i2c_read_bytes(uint8_t addr7, uint8_t *data, uint16_t len);

i2c_status_t i2c_write_reg(uint8_t addr7, uint8_t reg, const uint8_t *data, uint16_t len);
i2c_status_t i2c_read_reg(uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len);