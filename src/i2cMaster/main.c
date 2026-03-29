#include "i2cMaster.h"
#include "gpio.h"
#include <util/delay.h>

static void led_init(void) {
    gpio_pin_mode(PIN_D13, GPIO_OUTPUT);
    gpio_write(PIN_D13, GPIO_LOW);
}

static void led_pulse(uint8_t pulses) {
    while (pulses--) {
        gpio_toggle(PIN_D13);
        _delay_ms(80);
        gpio_toggle(PIN_D13);
        _delay_ms(80);
    }
}

// Scan all 7-bit addresses; blink once on each ACK
static void i2c_bus_scan(void) {
    for (uint8_t addr = 1; addr < 0x80; addr++) {
        i2c_status_t st = i2c_start_write(addr);
        if (st == I2C_OK) {
            led_pulse(1);
        }
        i2c_stop();        // always release the bus
        _delay_ms(4);
    }
}

int main(void) {
    i2c_init();
    led_init();

    while (1) {
        i2c_bus_scan();
        _delay_ms(500);
    }
    return 0;
}