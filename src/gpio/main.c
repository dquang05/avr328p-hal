#include "gpio.h"
#include <util/delay.h>

// Example 1: Simple LED blink
void example_led_blink(void) {
    // Configure PIN_D13 (built-in LED on Arduino Uno) as output
    gpio_pin_mode(PIN_D13, GPIO_OUTPUT);
    
    while (1) {
        gpio_write(PIN_D13, GPIO_HIGH);  // Turn LED ON
        _delay_ms(1000);
        gpio_write(PIN_D13, GPIO_LOW);   // Turn LED OFF
        _delay_ms(1000);
    }
}

// Example 2: LED blink with toggle
void example_led_toggle(void) {
    // Configure PIN_D13 as output
    gpio_pin_mode(PIN_D13, GPIO_OUTPUT);
    
    while (1) {
        gpio_toggle(PIN_D13);  // Toggle LED state
        _delay_ms(500);
    }
}

// Example 3: Button reading with pull-up resistor
void example_button_led(void) {
    // Configure PIN_D13 as output (LED)
    gpio_pin_mode(PIN_D13, GPIO_OUTPUT);
    
    // Configure PIN_D2 as input with internal pull-up
    // Button should connect PIN_D2 to GND
    gpio_pin_mode(PIN_D2, GPIO_INPUT_PULLUP);
    
    while (1) {
        // Read button state (active LOW with pull-up)
        int8_t button_state = gpio_read(PIN_D2);
        
        if (button_state == GPIO_LOW) {
            // Button is pressed (connected to GND)
            gpio_write(PIN_D13, GPIO_HIGH);
        } else {
            // Button is not pressed
            gpio_write(PIN_D13, GPIO_LOW);
        }
        _delay_ms(10);  // Debounce delay
    }
}

// Example 4: Multiple LEDs control
void example_multiple_leds(void) {
    // Configure pins D8-D11 as outputs
    gpio_pin_mode(PIN_D8, GPIO_OUTPUT);
    gpio_pin_mode(PIN_D9, GPIO_OUTPUT);
    gpio_pin_mode(PIN_D10, GPIO_OUTPUT);
    gpio_pin_mode(PIN_D11, GPIO_OUTPUT);
    
    while (1) {
        // Turn on LEDs in sequence
        gpio_write(PIN_D8, GPIO_HIGH);
        _delay_ms(200);
        gpio_write(PIN_D9, GPIO_HIGH);
        _delay_ms(200);
        gpio_write(PIN_D10, GPIO_HIGH);
        _delay_ms(200);
        gpio_write(PIN_D11, GPIO_HIGH);
        _delay_ms(200);
        
        // Turn off LEDs in sequence
        gpio_write(PIN_D8, GPIO_LOW);
        _delay_ms(200);
        gpio_write(PIN_D9, GPIO_LOW);
        _delay_ms(200);
        gpio_write(PIN_D10, GPIO_LOW);
        _delay_ms(200);
        gpio_write(PIN_D11, GPIO_LOW);
        _delay_ms(200);
    }
}

// Example 5: Digital input sensor
void example_sensor_reading(void) {
    // Configure PIN_A0 as input (floating)
    gpio_pin_mode(PIN_A0, GPIO_INPUT);
    
    // Configure PIN_D13 as output (status LED)
    gpio_pin_mode(PIN_D13, GPIO_OUTPUT);
    
    while (1) {
        int8_t sensor_value = gpio_read(PIN_A0);
        
        if (sensor_value == GPIO_HIGH) {
            gpio_write(PIN_D13, GPIO_HIGH);
        } else {
            gpio_write(PIN_D13, GPIO_LOW);
        }
        _delay_ms(50);
    }
}

// Example 6: Traffic light simulation
void example_traffic_light(void) {
    #define LED_RED   PIN_D10
    #define LED_YELLOW PIN_D11
    #define LED_GREEN PIN_D12
    
    // Configure LEDs as outputs
    gpio_pin_mode(LED_RED, GPIO_OUTPUT);
    gpio_pin_mode(LED_YELLOW, GPIO_OUTPUT);
    gpio_pin_mode(LED_GREEN, GPIO_OUTPUT);
    
    while (1) {
        // Red light
        gpio_write(LED_RED, GPIO_HIGH);
        gpio_write(LED_YELLOW, GPIO_LOW);
        gpio_write(LED_GREEN, GPIO_LOW);
        _delay_ms(3000);
        
        // Yellow light
        gpio_write(LED_RED, GPIO_LOW);
        gpio_write(LED_YELLOW, GPIO_HIGH);
        gpio_write(LED_GREEN, GPIO_LOW);
        _delay_ms(1000);
        
        // Green light
        gpio_write(LED_RED, GPIO_LOW);
        gpio_write(LED_YELLOW, GPIO_LOW);
        gpio_write(LED_GREEN, GPIO_HIGH);
        _delay_ms(3000);
    }
}

// Main function - uncomment the example you want to run
int main(void) {
    // Choose one example to run:
    
    // example_led_blink();           // Example 1: Basic blink
    example_led_toggle();       // Example 2: Blink using toggle
    // example_button_led();       // Example 3: Button control
    // example_multiple_leds();    // Example 4: Multiple LEDs
    // example_sensor_reading();   // Example 5: Digital sensor
    // example_traffic_light();    // Example 6: Traffic light
    
    return 0;
}
