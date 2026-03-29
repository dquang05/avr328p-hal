#include <avr/io.h>
#include <util/delay.h>
#include "gpio.h"

int main(void)
{
  gpio_pin_mode(28, GPIO_OUTPUT); // Set pin 28 (LED_BUILTIN) as output
  while (1)
  {
    gpio_toggle(28);
    _delay_ms(1000);
  }
  return 0;
}
