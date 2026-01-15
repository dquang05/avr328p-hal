#ifndef DRIVER_GPIO_H
#define DRIVER_GPIO_H
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>



typedef uint8_t gpio_pin_t;

typedef enum {
    GPIO_INPUT = 0,
    GPIO_INPUT_PULLUP,
    GPIO_OUTPUT
} gpio_mode_t;

typedef enum {
    GPIO_LOW = 0,
    GPIO_HIGH = 1
} gpio_level_t;

/* Arduino Uno pin numbers (recommended convention) */
enum {
    PIN_D0 = 0,  PIN_D1,  PIN_D2,  PIN_D3,  PIN_D4,  PIN_D5,  PIN_D6,  PIN_D7,
    PIN_D8,      PIN_D9,  PIN_D10, PIN_D11, PIN_D12, PIN_D13,
    PIN_A0 = 14, PIN_A1,  PIN_A2,  PIN_A3,  PIN_A4,  PIN_A5
};

/* Public API */
bool gpio_pin_mode(gpio_pin_t pin, gpio_mode_t mode);
bool gpio_write(gpio_pin_t pin, gpio_level_t level);
int8_t gpio_read(gpio_pin_t pin);      // return 0/1, or -1 if invalid pin
bool gpio_toggle(gpio_pin_t pin);


#endif

