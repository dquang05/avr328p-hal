#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "gpio.h"

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) {
    MCUSR = 0;
    wdt_disable();
}

int main(void) {
    gpio_pin_mode(PIN_D8, GPIO_OUTPUT);

    while (1) {
        gpio_write(PIN_D8, GPIO_HIGH);
        _delay_ms(100);
        gpio_write(PIN_D8, GPIO_LOW);
        _delay_ms(100);
    }
}