#include "gpio.h"
#include <avr/interrupt.h>

/*Save register of registers*/
typedef struct {
    volatile uint8_t* ddr;
    volatile uint8_t* port;
    volatile uint8_t* pin;
    uint8_t mask;
} gpio_map_t;

/*GPIO mapping for Arduino Uno*/
static const gpio_map_t gpio_map[] = {
//Structure: { Pinmode, Conf, Pin Status, bitmask }
    /* D0 - D7 */
    { &DDRD, &PORTD, &PIND, (1 << PD0) }, // PIN_D0
    { &DDRD, &PORTD, &PIND, (1 << PD1) }, // PIN_D1
    { &DDRD, &PORTD, &PIND, (1 << PD2) }, // PIN_D2
    { &DDRD, &PORTD, &PIND, (1 << PD3) }, // PIN_D3
    { &DDRD, &PORTD, &PIND, (1 << PD4) }, // PIN_D4
    { &DDRD, &PORTD, &PIND, (1 << PD5) }, // PIN_D5
    { &DDRD, &PORTD, &PIND, (1 << PD6) }, // PIN_D6
    { &DDRD, &PORTD, &PIND, (1 << PD7) }, // PIN_D7
    /* D8 - D13 */
    { &DDRB, &PORTB, &PINB, (1 << PB0) }, // PIN_D8
    { &DDRB, &PORTB, &PINB, (1 << PB1) }, // PIN_D9
    { &DDRB, &PORTB, &PINB, (1 << PB2) }, // PIN_D10
    { &DDRB, &PORTB, &PINB, (1 << PB3) }, // PIN_D11
    { &DDRB, &PORTB, &PINB, (1 << PB4) }, // PIN_D12
    { &DDRB, &PORTB, &PINB, (1 << PB5) }, // PIN_D13
    /* A0 - A5 */
    { &DDRC, &PORTC, &PINC, (1 << PC0) }, // PIN_A0
    { &DDRC, &PORTC, &PINC, (1 << PC1) }, // PIN_A1
    { &DDRC, &PORTC, &PINC, (1 << PC2) }, // PIN_A2
    { &DDRC, &PORTC, &PINC, (1 << PC3) }, // PIN_A3
    { &DDRC, &PORTC, &PINC, (1 << PC4) }, // PIN_A4
    { &DDRC, &PORTC, &PINC, (1 << PC5) }  // PIN_A5
};

const uint8_t GPIO_PIN_COUNT = sizeof(gpio_map) / sizeof(gpio_map_t);

bool gpio_pin_mode(gpio_pin_t pin, gpio_mode_t mode) {
    if (pin >= GPIO_PIN_COUNT) return false;

    const gpio_map_t *p = &gpio_map[pin];
    uint8_t sreg = SREG;
    cli(); // Protect against interrupts during register modification

    if (mode == GPIO_OUTPUT) {
        *(p->ddr) |= p->mask;
    } else {
        *(p->ddr) &= ~(p->mask);
        if (mode == GPIO_INPUT_PULLUP) {
            *(p->port) |= p->mask;
        } else {
            *(p->port) &= ~(p->mask);
        }
    }

    SREG = sreg; // Restore interrupt status
    return true;
}

bool gpio_write(gpio_pin_t pin, gpio_level_t level) {
    if (pin >= GPIO_PIN_COUNT) return false;

    const gpio_map_t *p = &gpio_map[pin];
    if (level == GPIO_HIGH) {
        *(p->port) |= p->mask;
    } else {
        *(p->port) &= ~(p->mask);
    }
    return true;
}

int8_t gpio_read(gpio_pin_t pin) {
    if (pin >= GPIO_PIN_COUNT) return -1;

    const gpio_map_t *p = &gpio_map[pin];
    return (*(p->pin) & p->mask) ? GPIO_HIGH : GPIO_LOW;
}

bool gpio_toggle(gpio_pin_t pin) {
    if (pin >= GPIO_PIN_COUNT) return false;

    const gpio_map_t *p = &gpio_map[pin];
    uint8_t sreg = SREG;
    cli();
    // This works because writing a 1 to PINx toggles the corresponding PORTx bit (i read it somewhere in the datasheet)
    *(p->pin) = p->mask; 
    SREG = sreg;
    return true;
}