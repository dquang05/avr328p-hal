#include "gpio.h"
#include <util/delay.h>

// Simple LED blink
void example_led_blink(void) {
    gpio_pin_mode(PIN_D13, GPIO_OUTPUT);
    
    while (1) {
        gpio_write(PIN_D13, GPIO_HIGH);  // Turn LED ON
        _delay_ms(1000);
        gpio_write(PIN_D13, GPIO_LOW);   // Turn LED OFF
        _delay_ms(1000);
    }
}


int main(void) {
    example_led_blink();
    return 0;
}

