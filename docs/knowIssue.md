## Troubleshooting: program uploaded successfully but never reaches `main()`

When flashing an Arduino Uno / ATmega328P with USBasp, the firmware may appear to upload successfully, but the application never actually enters `main()`. A typical symptom is that the onboard LED on D13 stays constantly ON, even if the user code contains an empty `while(1)` loop or a simple blink routine.

One possible cause is the Watchdog Timer (WDT) remaining active after reset. In this case, disabling WDT inside `main()` is too late, because the MCU may reset again before normal application startup completes.

### Fix
Disable the watchdog in the early startup stage using the `.init3` section, before `main()` is executed:

```cpp
#include <avr/io.h>
#include <avr/wdt.h>

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) {
    MCUSR = 0;
    wdt_disable();
}
```
This ensures the watchdog is cleared at startup and prevents the MCU from getting stuck in a reset loop before the main application begins (it can be observed by monitoring the behavior of PB5 - always HIGH).