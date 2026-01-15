#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include <stdint.h>
#include <avr/io.h>
// Default I2C settings
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef I2C_SCL_FREQ
#define I2C_SCL_FREQ 100000UL
#endif

#ifndef I2C_TIMEOUT
#define I2C_TIMEOUT 60000UL 
#endif

typedef enum {
    I2C_OK    = 0,
    I2C_NACK  = 1,
    I2C_ERROR = 2,
    I2C_TIMEOUT_ERR = 3
} i2c_status_t;

void i2c_init(void);

i2c_status_t i2c_start_write(uint8_t addr7);
i2c_status_t i2c_start_read(uint8_t addr7);
i2c_status_t i2c_restart_write(uint8_t addr7);
i2c_status_t i2c_restart_read(uint8_t addr7);

void i2c_stop(void);

i2c_status_t i2c_write(uint8_t data);

i2c_status_t i2c_read_ack(uint8_t *out);
i2c_status_t i2c_read_nack(uint8_t *out);

// Helpers 
i2c_status_t i2c_write_bytes(uint8_t addr7, const uint8_t *data, uint16_t len);
i2c_status_t i2c_read_bytes(uint8_t addr7, uint8_t *data, uint16_t len);

i2c_status_t i2c_write_reg(uint8_t addr7, uint8_t reg, const uint8_t *data, uint16_t len);
i2c_status_t i2c_read_reg(uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len);

#endif
